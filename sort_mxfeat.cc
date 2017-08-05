/************************
 * author : chunqi.wang@hobot.cc
 * time   : 2017-8-5
 ************************/

#include <fstream>
#include <iostream>
#include <algorithm> 
#include <cstdlib>
#include <vector>
#include <queue>
#include <string>
#include <cstring>
#include "unistd.h"
#include "dirent.h"

using namespace std;

class item_info {
  public:
    friend bool operator < (const item_info &a, const item_info &b) {
      return a.data_size > b.data_size;
    }

    string   path;
    uint32_t address;
    uint32_t data_size;
    uint32_t cflag;
};

vector <string> get_filelist (char* path) {
  vector <string> filelist;
  DIR     *dir = opendir(path);
  dirent  *ptr = NULL;
  while ((ptr = readdir(dir)) != NULL) {
    if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
      filelist.push_back(string(path) + '/' + string(ptr->d_name));
    }
  }
  return filelist;
}

void get_item_info (const string path, priority_queue <item_info> &sorted_filelist) {
  ifstream fin;
  fin.open(path, ios::binary);
  
  fin.seekg(0, ios::end);
  streampos end_pos = fin.tellg();
  fin.seekg(0, ios::beg);

  item_info info;
  uint32_t data_4bit;
  while (fin.tellg() != end_pos) { 
    fin.seekg(4, ios::cur); //pass the 4bit magic number
    fin.read(reinterpret_cast <char*> (&data_4bit), 4); //read the flag of the item
    info.path      = path;
    info.address   = fin.tellg();
    info.data_size = data_4bit & ((1U << 29U) - 1U);
    info.cflag     = (data_4bit>>29U) & 7U;
    sorted_filelist.push(info);
    fin.seekg(info.data_size, ios::cur);
  }
  fin.close();
  cout << "reading now, total " << sorted_filelist.size() << " mxfeat items have been read" << endl;
}

void write_file (const vector<item_info> itemlist, const int items_per_file, const string dest_dir, const string prefix) {
  int len = itemlist.size();
  if (len == 0) return;

  item_info info;
  char *buf         = new char[650000000];
  int  n_items      = 0;
  int  out_file_id  = 0;

  ifstream fin;
  ofstream fout;
  fout.open(dest_dir + '/' + prefix + "id_" + to_string(out_file_id), ios::binary);
  for (int i = 0; i < len; i++) {
    info = itemlist[i];
    uint32_t magic_number    = 0xced7230a;
    uint32_t flag_and_length = (info.cflag << 29U) | info.data_size;
    fin.open(info.path, ios::binary);
    fin.seekg(info.address, ios::beg);
    fin.read(buf, info.data_size);
    fin.close();

    fout.write(reinterpret_cast <char*> (&magic_number), 4);
    fout.write(reinterpret_cast <char*> (&flag_and_length), 4);
    fout.write(buf, info.data_size);
    n_items += 1;

    if (n_items % items_per_file == 0) {
      out_file_id += 1;
//      cout << "items " << prefix << " finished " << n_items*100/1.0/len << "%" << endl;
      fout.close();
      if (n_items != len) fout.open(dest_dir + '/' + prefix + "id_" + to_string(out_file_id), ios::binary);
    }
  }
  cout << "items  " << prefix << " finished 100%" << endl;
  fout.close();
}


void rebuild_mxfeats (
    const priority_queue <item_info>  &sorted_filelist, 
    const vector <int>                &split_points, 
    const string                      dest_dir, 
    const int                         items_per_file,
    const int                         feats_dim) {
  auto      locallist = sorted_filelist;
  int       n_points  = split_points.size();
  int       ptr_parts = 1;
  item_info info;
  string    prefix;
  vector <item_info> * parts = new vector<item_info>[split_points.size()];

  prefix = "length_" + to_string(split_points[ptr_parts-1]) + '_';
  if (ptr_parts == n_points-1) prefix += "max_";
  else prefix += to_string(split_points[ptr_parts]) + '_';

  cout << endl;
  cout << "job started, total " << locallist.size() << " mxfeat items" << endl;
  while (!locallist.empty()) {
    info = locallist.top();
    while (info.data_size > split_points[ptr_parts]*(feats_dim+1)*4) {
      write_file(parts[ptr_parts], items_per_file, dest_dir, prefix);
      cout << "total progress finished " << (1-(locallist.size()/1.0/sorted_filelist.size()))*100 << "%" << endl <<endl;
      ptr_parts++;

      prefix = "length_" + to_string(split_points[ptr_parts-1]) + '_';
      if (ptr_parts == n_points-1) prefix += "max_";
      else prefix += to_string(split_points[ptr_parts]) + '_';
    }
    parts[ptr_parts].push_back(info);
    locallist.pop();
  }
  write_file(parts[ptr_parts], items_per_file, dest_dir, prefix);
  cout << "job finished, have a nice day o(^.^o)" << endl;
}

int main (int argcnt, char **argval) {
  if (argcnt <= 5) {
    cout << "sort mxfeats and rebuild mxfeats file" << endl;
    cout << "usage: " << argval[0] << " <src_dir> <dest_dir> <items_per_file> <feats_dim> [split_point] [split_point] .....     " << endl;
    cout << "e.g. : " << argval[0] << " ./path_to_mxfeats_dir ./path_to_store_new_mxfeats 5000 100 200 300 400 500              " << endl;
    cout << "note : if split point is 200 and 300, the item(data chunk) whose length is smaller than 200 will                   " << endl;
    cout << "       be saved in file \"length0_200\", length between 200 and 300 will be saved in \"length200_300\"             " << endl;
    cout << "       the others will be saved in \"length300_650000000\"                                                         " << endl;
    cout << "warning : 1. only support 4-bit label and 4-bit feats data. which means label data type should be uint32_t         " << endl;
    cout << "             and feats data type should be float32. if not, data set will be destroyed and can`t be used any more  " << endl;
    cout << "          2. if dim is not correct, program will NOT classify item`s frame length correctly. For example, an item  " << endl;
    cout << "             with real frame quantity 400 will be classified to 200 frames data set if you give the program        " << endl;
    cout << "             a 2 times larger dim than its real dim. but output data set still could be used.                      " << endl;
    return 0;
  }
  if (opendir(argval[1]) == NULL) {
    cout << "ERROR : src_dir not exist" << endl;
    return 0;
  }
  if (opendir(argval[2]) == NULL) {
    cout << "ERROR : dest_dir not exist" << endl;
    return 0;
  }
  if (atoi(argval[4]) != 40 && atoi(argval[4]) != 60 && atoi(argval[4]) != 80)
    cout << "feats dim is not 40, 60 or 80, ARE YOU SURE?" << endl;

  vector <string> filelist = get_filelist(argval[1]);
  priority_queue <item_info> sorted_filelist; 
  for (string val : filelist) get_item_info(val, sorted_filelist);

  vector <int> split_points;
  split_points.push_back(0); 
  for (int i = 5; i < argcnt; i++) split_points.push_back(atoi(argval[i]));
  split_points.push_back(5000000); 

  rebuild_mxfeats (sorted_filelist, split_points, argval[2], atoi(argval[3]), atoi(argval[4]));
  return 0;
}

  
