/*************************************************************************
  > File Name:    kmeans.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Fri 28 Aug 2020 03:52:26 PM CST
 ************************************************************************/

#ifndef KMEANS_H_
#define KMEANS_H_

#include <fstream>
#include <string.h>
#include <vector>

namespace base {

class KMeans {
 public:
  enum InitMode {
    InitRandom,
    InitManual,
    InitUniform,
  };

  explicit KMeans(int dim_num = 1, int cluster_num = 1);
  ~KMeans();

  void SetMean(int i, const double* u) {
    memcpy(means_.data() + i * dim_num_, u, sizeof(double) * dim_num_);
  }
  void SetInitMode(int i) { init_mode_ = i; }
  void SetMaxIterNum(int i) { max_iter_num_ = i; }
  void SetEndError(double f) { end_error_ = f; }

  double* means(int i) { return (means_.data() + i * dim_num_); }
  int init_mode() { return init_mode_; }
  int max_iter_num() { return max_iter_num_; }
  double end_error() { return end_error_; }

  /* SampleFile: <size><dim><sample_data><sample_data>...
   *              int   int    double   seq_data = dim * seq_size
   *  size: numble of samples
   *  dim: dimension of feature
   */
  void Cluster(const char* sample_filename, const char* label_filename);
  void Init(std::ifstream& sample_file);
  void Cluster(double* data, int num, int* lable);
  void Init(double* data, int num);
  friend std::ostream& operator << (std::ostream& out, KMeans& kmeans);

 private:
  double GetLable(const double* x, int* label);
  double CalcDistance(const double* x, const double* y, int dim_num);

  int dim_num_;
  int cluster_num_;
  int init_mode_;
  int max_iter_num_;

  double end_error_;
  std::vector<double> means_;
};

}  // namespace base

#endif
