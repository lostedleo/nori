/*************************************************************************
  > File Name:    kmeans_test.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Fri 28 Aug 2020 06:20:22 PM CST
 ************************************************************************/

#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>

#include "algorithm/hmm/kmeans.h"

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
  const int size = 10; //Number of samples
  const int dim = 3;   //Dimension of feature
  const int cluster_num = 4; //Cluster number

  base::KMeans* kmeans = new base::KMeans(dim, cluster_num);
  int* labels = new int[size];
  kmeans->SetInitMode(base::KMeans::InitUniform);
  kmeans->Cluster(data, size, labels);
  std::stringstream ss;
  ss << *kmeans;
  std::cout << ss.str();

  for(int i = 0; i < size; ++i)
  {
    printf("%f, %f, %f belongs to %d cluster\n", data[i*dim+0], data[i*dim+1], data[i*dim+2], labels[i]);
  }

  delete []labels;
  delete kmeans;
}

void TestFile() {
  const int size = 10; //Number of samples
  const int dim = 3;   //Dimension of feature
  const int cluster_num = 4; //Cluster number

  std::string sample_filename = "sample.txt";
  std::ofstream sample_file(sample_filename.c_str(), std::ios_base::binary);
  sample_file.write((char*)&size, sizeof(int));
  sample_file.write((char*)&dim, sizeof(int));
  sample_file.write((char*)data, sizeof(double) * size * dim);
  sample_file.close();

  std::string label_filename = "label.txt";
  base::KMeans kmeans = base::KMeans(dim, cluster_num);
  kmeans.SetInitMode(base::KMeans::InitUniform);
  kmeans.Cluster(sample_filename.c_str(), label_filename.c_str());
  std::stringstream ss;
  ss << kmeans;
  std::cout << ss.str();

  std::ifstream label_file(label_filename.c_str(), std::ios_base::binary);
  label_file.seekg(sizeof(int), std::ios_base::beg);
  int label = -1;

  for(int i = 0; i < size; ++i) {
    label_file.read((char*)&label, sizeof(int));
    printf("%f, %f, %f belongs to %d cluster\n", data[i*dim+0], data[i*dim+1], data[i*dim+2], label);
  }

  remove(sample_filename.c_str());
  remove(label_filename.c_str());
}

int main(int argc, char** argv) {
  TestData();
  TestFile();

  return 0;
}
