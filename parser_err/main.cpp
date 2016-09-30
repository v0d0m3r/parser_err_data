/*
Входная информация: файл в кодировке Windows-1251.
Цель: перекодировка (из win 1251 в utf8) и поиск ошибок из файлов.
Структура файловой системы:
Папка, в которой находиться программа, должна содержать
1) папки для каждого объекта, в которых входные файлы
2) папка с именем result, должна быть пустой!!!
./this_program
./folder1/file_name1
./folder2/file_name2
./folderN/file_nameM
./reusult/
3) выходной файл (пример)
./reusult/folder2_err_out
Алгоритм:
1. проверяем поддирректории
2. ищем в поддиректории файлы для обработки
3. построчно перебираем каждый файл и находим ошибки
4. заполняем вектор ошибок
5. расчитываем статистику и выводим вектор ошибок в файлы

Грамматика для строк входной информации:
1) какая_то_информация, дата, время, код, описание, какая_то_информация
2) какая_то_информация, код, описание, какая_то_информация
3432432,234243,010216,232213,005,ошибка,234234
На выходе:
010216;232213;005;ошибка
*/

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdio.h>
#include <fcntl.h>
#include <stdexcept>
#include <ctime>
#include <iostream>
#include "fstream"
#include "sstream"
#include "dirent.h"
#include <iomanip>

//------------------------------------------------------------------------------

using namespace std;

//------------------------------------------------------------------------------

static void cp1251_to_utf8(char *out, const char *in) {
    static const int table[128] = {
        0x82D0,0x83D0,0x9A80E2,0x93D1,0x9E80E2,0xA680E2,0xA080E2,0xA180E2,
        0xAC82E2,0xB080E2,0x89D0,0xB980E2,0x8AD0,0x8CD0,0x8BD0,0x8FD0,
        0x92D1,0x9880E2,0x9980E2,0x9C80E2,0x9D80E2,0xA280E2,0x9380E2,0x9480E2,
        0,0xA284E2,0x99D1,0xBA80E2,0x9AD1,0x9CD1,0x9BD1,0x9FD1,
        0xA0C2,0x8ED0,0x9ED1,0x88D0,0xA4C2,0x90D2,0xA6C2,0xA7C2,
        0x81D0,0xA9C2,0x84D0,0xABC2,0xACC2,0xADC2,0xAEC2,0x87D0,
        0xB0C2,0xB1C2,0x86D0,0x96D1,0x91D2,0xB5C2,0xB6C2,0xB7C2,
        0x91D1,0x9684E2,0x94D1,0xBBC2,0x98D1,0x85D0,0x95D1,0x97D1,
        0x90D0,0x91D0,0x92D0,0x93D0,0x94D0,0x95D0,0x96D0,0x97D0,
        0x98D0,0x99D0,0x9AD0,0x9BD0,0x9CD0,0x9DD0,0x9ED0,0x9FD0,
        0xA0D0,0xA1D0,0xA2D0,0xA3D0,0xA4D0,0xA5D0,0xA6D0,0xA7D0,
        0xA8D0,0xA9D0,0xAAD0,0xABD0,0xACD0,0xADD0,0xAED0,0xAFD0,
        0xB0D0,0xB1D0,0xB2D0,0xB3D0,0xB4D0,0xB5D0,0xB6D0,0xB7D0,
        0xB8D0,0xB9D0,0xBAD0,0xBBD0,0xBCD0,0xBDD0,0xBED0,0xBFD0,
        0x80D1,0x81D1,0x82D1,0x83D1,0x84D1,0x85D1,0x86D1,0x87D1,
        0x88D1,0x89D1,0x8AD1,0x8BD1,0x8CD1,0x8DD1,0x8ED1,0x8FD1
    };
    while (*in)
        if (*in & 0x80) {
            int v = table[(int)(0x7f & *in++)];
            if (!v)
                continue;
            *out++ = (char)v;
            *out++ = (char)(v >> 8);
            if (v >>= 16)
                *out++ = (char)v;
        }
        else
            *out++ = *in++;
    *out = 0;
}

//------------------------------------------------------------------------------

static void get_string_utf8(string& line)
{
    char in[line.size()+1];
    for (int i=0; i < line.size(); ++i) in[i] = line[i];
    in[line.size()] = '\0';

    char res[sizeof(in) * 3 + 1];
    cp1251_to_utf8(res, in);

    line = res;
}

//------------------------------------------------------------------------------

inline void keep_window_open(string str1)
{
  cout << "Чтобы выйти, введите " << str1 << '\n';
  for (string str2; cin>>str2;)
     if (str1 == str2)  { break; }
}

//------------------------------------------------------------------------------

void error(string from_err, string err)
{
  throw runtime_error(from_err+": "+ err);
}

//------------------------------------------------------------------------------
// Грубая проверка, является строка датой или временем
bool is_date_time(const string& str)
{
    static const int dt_size = 6;
    if (str.size() != dt_size) return false;
    for (const char& ch : str)
        if(!isdigit(ch)) return false;
    return true;
}

//------------------------------------------------------------------------------

vector<string> succes_code_tb {
    "000", "003"
};

//------------------------------------------------------------------------------

vector<string> error_code_tb {
    "002", "005", "020", "100", //"10" убрал
    "101", "103", "104", "105", "106",
    "107", "109", "110", "116", "111",
    "117", "118", "119", "120", "121",
    "123", "125", "126", "127", "128",
    "200", "202", "201", "202", "203",
    "204", "205", "206", "208", "209",
    "210", "212", "213", "214", "215",
    "216", "221", "222", "223", "224",
    "225", "226", "227", "228", "229",
    "230", "231", "232", "233", "234",
    "236", "301", "302", "303", "304",
    "305", "306", "401", "402", "403",
    "404", "410", "500", "902", "903",
    "904", "905", "907", "908", "909",
    "910", "911", "912", "913", "914",
    "915", "920", "923", "940", "952",
    "953", "954", "955", "956", "957",
    "958", "959", "960", "961", "962",
    "963", "964", "965", "966", "967",
    "968", "969", "970", "971", "972",
    "973", "974", "975", "976", "977",
    "978", "979", "980", "981", "982",
    "983", "984", "985", "986", "987",
    "988", "989", "990", "991", "992",
    "993", "994", "995", "996", "997",
    "998", "999"
};

//------------------------------------------------------------------------------

void parser_line(string& line);

//------------------------------------------------------------------------------
// Ввод из файла
void fill_from_file(vector<string>& error_tb, const string& name,
                    int& count_all)
{
    ifstream ist {name};    // Поток для чтения из файла
    if (!ist) error("fill_from_file(): невозможно открыть файл: ", name);
    string line;
    while (true) {
        getline(ist, line);
        if(!ist) break;

        get_string_utf8(line);  // Изменяем кодировку

        parser_line(line);      // Анализируем строку на ошибки
        if (line != "") error_tb.push_back(line);
        ++count_all;
    }
}

//------------------------------------------------------------------------------
// get_list_files() формирует пути файлов из sub_dir
void get_list_files(vector<string>& path_to_data_tb,
                    const string& sub_dir)
{
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir (sub_dir.c_str())) == NULL)
        error("get_list_files","Не удалось получить директорию!");

    while ((ent = readdir (dir)) != NULL)
        if (ent->d_type != DT_DIR)  // Отбрасываем директории
            path_to_data_tb.push_back(ent->d_name); // Добавляем имя файла

    closedir (dir);
    if (path_to_data_tb.size() == 0)
        error("get_list_files:", "Отсутствуют файлы");
    // Сортируем имена файлов
    sort(path_to_data_tb.begin(), path_to_data_tb.end());
    for (string& pd : path_to_data_tb)  // Формируем пути к файлам
        pd = "./" + sub_dir + '/' + pd;
}

//------------------------------------------------------------------------------
// get_subdir() получает таблицу поддиректорий
void get_subdir(vector<string>& subdir_tb)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("./")) == NULL)
        error("get_list_files","Не удалось получить директорию!");

    while ((ent = readdir (dir)) != NULL)
        if (ent->d_type == DT_DIR           // Отбрасываем директории,
                && ent->d_name[0] != '.'    // начинающие на '.'
                && ent->d_name[0] != 'r')   // и на 'r' - для папки с
                                            // результатом
            subdir_tb.push_back(ent->d_name);

    closedir (dir);
    if (subdir_tb.size() == 0)
        error("get_subdir:", "Отсутствуют поддиректории");    
}

//------------------------------------------------------------------------------

bool is_succes_code(const string& code)
{
    for (const string& sc : succes_code_tb)
        if (code == sc) return true;
    return false;
}

//------------------------------------------------------------------------------

bool is_error_code(const string& code)
{
    for (const string& ec : error_code_tb)
        if (code == ec) return true;
    return false;
}

//------------------------------------------------------------------------------
// Парсим данные из строки
void parser_line(string& line)
{
    for (char& ch : line)   // ',' является разделителем
        if (ch == ',')
            ch = ' ';       // Заменяем на пробелы
    stringstream ss(line);  // Поток позволяет работать
                            // с пробельными символами
    line = "";
    vector<string> word_tb;
    for (string word; ss >> word;)  // Разделяем всю строку на слова
        word_tb.push_back(word);
    // Ищем ошибки с датой и временем
    for (int i=0; i < word_tb.size(); ++i)
        if (is_date_time(word_tb[i])  // Первый дата и второй время?
                && is_date_time(word_tb[i+1])) {
            if (!is_succes_code(word_tb[i+2])) {    // Успешный ли код?
                line += word_tb[i] + ";";           // Разделяем запятыми
                line += word_tb[i+1] + ";";
                line += word_tb[i+2] + ";";
                // Проверяем, есть ли описание ошибки?
                line += (word_tb[i+3] == "\\x80\\x80\\x80") ?
                            "" : word_tb[i+3];
            }
            else return;
        }
    if (line != "") return;
    // Ищем ошибки без даты и времени
    for (int i=0; i < word_tb.size(); ++i)
        if(is_error_code(word_tb[i])
                && i!=word_tb.size()-1) {   // У нас ошибочный код?
            line += word_tb[i] + ";";
            line += word_tb[i+1];
        }
}

//------------------------------------------------------------------------------
// Вывод в файл
void fill_on_file(const vector<string>& error_tb, const string& name,
                  int count_all)
{
    ofstream ost {name, ios_base::app};
    if (!ost) error("fill_on_file: невозможно открыть файл:", name);
    for (const string& line : error_tb)
        ost << line << '\n';
    ost << "Всего: " << count_all << '\n'
        << "Ошибочно: " << error_tb.size() << '\n'
        << "Процент ошибок: " << setprecision(3)
        << double(error_tb.size() * 100.00 / count_all) << '\n'
        << "Успешно: " << count_all - error_tb.size() << '\n';
}

//------------------------------------------------------------------------------

void find_err()
{
    vector<string> subdir_tb;
    get_subdir(subdir_tb);              // Получаем поддиректории
    vector<string> path_to_data_tb;     // Пути до файлов

    const string oname = "err_out";
    string ofname;
    int count_all = 0;
    vector<string> error_tb;            // Таблица ошибок
    for (int i=0; i < subdir_tb.size(); ++i) {
        get_list_files(path_to_data_tb, subdir_tb[i]);
        for (string& data : path_to_data_tb)
            fill_from_file(error_tb, data, count_all); // Обрабатываем входные
                                                        // данные
        ofname = "./result/" + subdir_tb[i] + '_' + oname;
        fill_on_file(error_tb, ofname, count_all);      // Вывод ошибок в файл
        count_all = 0;
        error_tb.clear();
        path_to_data_tb.clear();
    }    
}

//------------------------------------------------------------------------------

int main()
try {
    find_err();
    keep_window_open("~~");
    return 0;
}
catch(exception& e) {
    cerr << e.what() << '\n';
    keep_window_open("~~");
    return -1;
}
catch (...) {
    cout << "exiting\n";
    keep_window_open("~");
    return -2;
}

//------------------------------------------------------------------------------
