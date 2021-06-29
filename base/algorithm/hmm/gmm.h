/*************************************************************************
  > File Name:    gmm.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Tue 01 Sep 2020 06:37:42 PM CST
 ************************************************************************/

#ifndef GMM_H_
#define GMM_H_

#include <fstream>
#include <vector>

namespace base {

class GMM {
 public:
  explicit GMM(int dim_num = 1, int mix_num = 1);
  ~GMM();

  void Copy(GMM* gmm);
  void SetMaxIterNum(int i) { max_iter_num_ = i; }
  void SetEndError(double f) { end_error_ = f; }

  int dim_num() { return dim_num_; }
  int mix_num() { return mix_num_; }
  int max_iter_num() { return max_iter_num_; }
  double end_error() { return end_error_; }

  double& prior(int i) { return priors_[i]; }
  double* means(int i) { return means_.data() + i * dim_num_; }
  double* vars(int i) { return vars_.data() + i * dim_num_; }

  void SetPrior(int i, double val) { priors_[i] = val; }
  void SetMean(int i, double* val) { for (int j = 0; j < dim_num_; ++j) means(i)[j] = val[j]; }
  void SetVariance(int i, double* val) { for (int j = 0; j < dim_num_; ++j) vars(i)[j]= val[j]; }

  double GetProbability(const double* sample);

  /* SampleFile: <size><dim><sample_data><sample_data>...
   *              int   int    double   seq_data = dim * seq_size
   *  size: numble of samples
   *  dim: dimension of feature
   */
  void Init(const char* sample_filename);
  void Train(const char* sample_filename);
  void Init(double* data, int num);
  void Train(double* data, int num);

  friend std::ostream& operator << (std::ostream& out, GMM& gmm);
  friend std::istream& operator >> (std::istream& in, GMM& gmm);

 private:
  double GetProbability(const double* x, int j);
  void Allocate();
  void Free();

  int dim_num_;
  int mix_num_;
  int max_iter_num_;

  double end_error_;
  std::vector<double> priors_;
  std::vector<double> means_;
  std::vector<double> vars_;
  std::vector<double> min_vars_;
};

}  // namespace base

#endif
