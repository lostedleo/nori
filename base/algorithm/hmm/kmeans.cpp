/*************************************************************************
  > File Name:    kmeans.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Fri 28 Aug 2020 03:53:27 PM CST
 ************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <assert.h>

#include "algorithm/hmm/kmeans.h"

namespace base {

KMeans::KMeans(int dim_num/*=1*/, int cluster_num/*=1*/) {
  dim_num_ = dim_num;
  cluster_num_ = cluster_num;
  means_.resize(cluster_num_ * dim_num_);

  init_mode_ = InitRandom;
  max_iter_num_ = 100;
  end_error_ = 0.001;
}

KMeans::~KMeans() {
}

void KMeans::Cluster(const char* sample_filename, const char* label_filename) {
  std::ifstream sample_file(sample_filename, std::ios_base::binary);
  assert(sample_file);

  int size = 0;
  int dim = 0;
  sample_file.read((char*)&size, sizeof(int));  // NOLINT
  sample_file.read((char*)&dim, sizeof(int));  // NOLINT
  assert(size >= cluster_num_);
  assert(dim == dim_num_);

  std::vector<double> data(size * dim, 0);
  sample_file.read((char*)data.data(), sizeof(double) * size * dim);  // NOLINT

  std::vector<int> labels(size, 0);
  Cluster(data.data(), size, labels.data());

  std::ofstream label_file(label_filename, std::ios_base::binary);
  assert(label_file);

  label_file.write((char*)&size, sizeof(int));  // NOLINT
  label_file.write((char*)labels.data(), sizeof(int) * size);  // NOLINT

  sample_file.close();
  label_file.close();
}

void KMeans::Init(std::ifstream& sample_file) {
  int size = 0;
  sample_file.seekg(0, std::ios_base::beg);
  sample_file.read((char*)&size, sizeof(int));  // NOLINT

  if (InitRandom == init_mode_) {
    int interval = size / cluster_num_;
    srand((unsigned)time(NULL));

    for (int i = 0; i < cluster_num_; ++i) {
      int select = interval * i + (interval - 1) * rand() / RAND_MAX;  // NOLINT
      int offset = sizeof(int) * 2 + select * sizeof(double) * dim_num_;

      sample_file.seekg(offset, std::ios_base::beg);
      sample_file.read((char*)means(i), sizeof(double) * dim_num_);  // NOLINT
    }
  } else if (InitUniform == init_mode_) {
    for (int i = 0; i < cluster_num_; ++i) {
      int select = i * size / cluster_num_;
      int offset = sizeof(int) * 2 + select * sizeof(double) * dim_num_;

      sample_file.seekg(offset, std::ios_base::beg);
      sample_file.read((char*)means(i), sizeof(double) * dim_num_);  // NOLINT
    }
  } else if (InitManual == init_mode_) {
    // Do nothing
  }
}

void KMeans::Cluster(double* data, int num, int* out_label) {
  assert(num >= cluster_num_);
  // First initialize model
  Init(data, num);

  // Second recursion
  int label = -1;
  double iter_num = 0, last_cost = 0, curr_cost = 0;
  int unchanged = 0;
  bool loop = true;
  std::vector<int> counts(cluster_num_, 0);
  std::vector<double> next_means(cluster_num_ * dim_num_, 0);

  while (loop) {
    last_cost = curr_cost;
    curr_cost = 0;
    // Classification
    for (int i = 0; i < num; ++i) {
      curr_cost += GetLable(data + i * dim_num_, &label);
      counts[label]++;

      for (int j = 0; j < dim_num_; ++j) {
        next_means[label * dim_num_ + j] += data[i * dim_num_ + j];
      }
    }
    curr_cost /= num;

    // Reestimation
    for (int i = 0; i < cluster_num_; ++i) {
      if (counts[i] > 0) {
        for (int j = 0; j < dim_num_; ++j) {
          next_means[i * dim_num_ + j] /= counts[i];
        }
        memcpy(means(i), next_means.data() + i * dim_num_, sizeof(double) * dim_num_);
      }
    }

    // Terminal conditions
    iter_num++;
    if (fabs(last_cost - curr_cost) < end_error_ * last_cost) {
      unchanged++;
    }
    if (iter_num >= max_iter_num_ || unchanged >= 3) {
      loop = false;
    }
    memset(counts.data(), 0, sizeof(int) * cluster_num_);
    memset(next_means.data(), 0, sizeof(double) * cluster_num_ * dim_num_);

    // For debug
    // std::cout << "Iter: " << iter_num << " unchanged: " << unchanged << ", Average Cost: "
    //  << curr_cost << std::endl;
  }

  for (int i = 0; i < num; ++i) {
    GetLable(data + i * dim_num_, &label);
    out_label[i] = label;
  }
}

void KMeans::Init(double* data, int num) {
  if (InitRandom == init_mode_) {
    int interval = num / cluster_num_;
    srand((unsigned)time(NULL));
    int select;
    for (int i = 0; i < cluster_num_; ++i) {
      select = interval * i + (interval - 1) * rand() / RAND_MAX;  // NOLINT
      memcpy(means(i), data + select * dim_num_, sizeof(double) * dim_num_);
    }
  } else if (InitUniform == init_mode_) {
    int select;
    for (int i = 0; i < cluster_num_; ++i) {
      select = i * num / cluster_num_;
      memcpy(means(i), data + select * dim_num_, sizeof(double) * dim_num_);
    }
  } else {
    // Do nothing
  }
}

double KMeans::GetLable(const double* sample, int* label) {
  double dist = -1;
  for (int i = 0; i < cluster_num_; ++i) {
    double temp = CalcDistance(sample, means(i), dim_num_);
    if (temp < dist || dist == -1) {
      dist = temp;
      *label = i;
    }
  }
  return dist;
}

double KMeans::CalcDistance(const double* x, const double* y, int dim_num) {
  double temp = 0;
  for (int i = 0; i < dim_num; ++i) {
    temp += (x[i] - y[i]) * (x[i] - y[i]);
  }
  return sqrt(temp);
}

std::ostream& operator << (std::ostream& out, KMeans& kmeans) {
  out << "<KMeans>" << std::endl;
  out << "<DimNum> " << kmeans.dim_num_ << " </DimNum>" << std::endl;
  out << "<ClusterNum> " << kmeans.cluster_num_ << " </ClusterNum>" << std::endl;
  out << "<Mean>" << std::endl;
  for (int i = 0; i < kmeans.cluster_num_; ++i) {
    for (int j = 0; j < kmeans.dim_num_; ++j) {
      out << kmeans.means_[i * kmeans.dim_num_ + j] << " ";
    }
    out << std::endl;
  }
  out << "</Mean>" << std::endl;
  out << "</KMeans>" << std::endl;

  return out;
}

}  // namespace base
