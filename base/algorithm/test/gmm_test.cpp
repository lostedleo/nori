/*************************************************************************
  > File Name:    gmm_test.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Wed 02 Sep 2020 02:36:44 PM CST
 ************************************************************************/

#include <iostream>
#include <fstream>
#include <assert.h>

#include "algorithm/hmm/kmeans.h"
#include "algorithm/hmm/gmm.h"

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

const int size = 10; //Number of samples
const int dim = 3;   //Dimension of feature

double test_data[4][3] = {
  {0.1, 0.2, 0.3,},
  {0.4, 0.5, 0.6,},
  {5.0, 6.2, 8.4,},
  {10.3, 10.4, 10.5,},
};

void TestData() {
  //Test GMM
  base::GMM *gmm = new base::GMM(dim, 3); //GMM has 3 SGM
  gmm->Train(data, size); //Training GMM

  printf("Test GMM:\n");
  for(int i = 0; i < 4; ++i)
  {
    printf("The Probability of %f, %f, %f  is %f \n", test_data[i][0], test_data[i][1],
        test_data[i][2], gmm->GetProbability(test_data[i]));
  }

  //save GMM to file
  std::ofstream gmm_file("gmm.txt");
  assert(gmm_file);
  gmm_file<<*gmm;
  gmm_file.close();

  remove("gmm.txt");
  delete gmm;
}

void TestFile() {
  std::string sample_filename = "sample.txt";
  std::ofstream sample_file(sample_filename.c_str(), std::ios_base::binary);
  sample_file.write((char*)&size, sizeof(int));
  sample_file.write((char*)&dim, sizeof(int));
  sample_file.write((char*)data, sizeof(double) * size * dim);
  sample_file.close();

  base::GMM gmm = base::GMM(dim, 3);
  gmm.Train(sample_filename.c_str());

  printf("Test GMM:\n");
  for(int i = 0; i < 4; ++i)
  {
    printf("The Probability of %f, %f, %f  is %f \n", test_data[i][0], test_data[i][1],
        test_data[i][2], gmm.GetProbability(test_data[i]));
  }

  //save GMM to file
  std::ofstream gmm_file("gmm.txt");
  assert(gmm_file);
  gmm_file << gmm;
  gmm_file.close();

  remove("gmm.txt");
}

int main() {
  TestData();
  TestFile();

  return 0;
}
