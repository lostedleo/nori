/*************************************************************************
  > File Name:    kmeans_test.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Fri 28 Aug 2020 06:20:22 PM CST
 ************************************************************************/

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <string>
#include <string.h>

#include "algorithm/hmm/hmm.h"

double data[] = {
  0.0, 0.2, 0.4,
  0.3, 0.2, 0.4,
  0.4, 0.2, 0.4,
  0.5, 0.2, 0.4,
  5.0, 5.2, 8.4,
  6.0, 5.2, 7.4,
  4.0, 5.2, 4.4,
  10.3, 10.4, 10.5,
  10.1, 10.6, 10.7,
  11.3, 10.2, 10.9
};


void TestData() {
  const int dim = 3;   //Dimension of feature
  std::vector<int> seq = {2, 3, 4, 1};

  int index = 0;
  std::vector<std::vector<double> > sample_data;
  int seq_size = seq.size();
  sample_data.resize(seq_size);
  for (int i = 0; i < seq_size; ++i) {
    sample_data[i].resize(seq[i] * dim);
    memcpy(sample_data[i].data(), data + index * dim, sizeof(double) * dim * seq[i]);
    index += seq[i];
  }

  base::HMM hmm = base::HMM(4, 3, 1);
  hmm.Train(sample_data);

  // save HMM to file
  std::ofstream hmm_file("hmm.txt");
  assert(hmm_file);
  hmm_file << hmm;
  hmm_file.close();

  remove("hmm.txt");
}

void TestFile() {
  const int dim = 3;

  std::string sample_filename = "sample.txt";
  std::vector<int> seq = {2, 3, 4, 1};
  int size = seq.size();
  std::ofstream sample_file(sample_filename.c_str(), std::ios_base::binary);
  sample_file.write((char*)&size, sizeof(int));
  sample_file.write((char*)&dim, sizeof(int));

  int index = 0;
  for (int i = 0; i < (int)seq.size(); ++i) {
    sample_file.write((char*)&seq[i], sizeof(int));
    sample_file.write((char*)(data+index*dim), sizeof(double) * dim * seq[i]);
    index += seq[i];
  }
  sample_file.close();

  base::HMM hmm = base::HMM(4, 3, 1);
  hmm.Train(sample_filename.c_str());

  // save HMM to file
  std::ofstream hmm_file("hmm.txt");
  assert(hmm_file);
  hmm_file << hmm;
  hmm_file.close();

  remove("hmm.txt");
  remove(sample_filename.c_str());
}

int main(int argc, char** argv) {
  TestData();
  TestFile();

  return 0;
}
