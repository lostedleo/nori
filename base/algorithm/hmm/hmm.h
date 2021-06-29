/*************************************************************************
  > File Name:    hmm.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Fri 04 Sep 2020 11:42:01 AM CST
 ************************************************************************/

#ifndef HMM_H_
#define HMM_H_

#include <vector>
#include "algorithm/hmm/hmm.h"

namespace base {

class GMM;

class HMM {
 public:
  explicit HMM(int state_num = 1, int dim_num = 1, int mix_num = 1);
  ~HMM();

  int state_num() { return state_num_; }
  int max_iter_num() { return max_iter_num_; }
  double end_error() { return end_error_; }

  void SetMaxIterNum(int n) { max_iter_num_ = n; }
  void SetEndError(double e) { end_error_ = e; }

  GMM* GetStateModel(int index);
  double GetStateInit(int index);
  double GetStateFinal(int index);
  double GetStateTrans(int i, int j);

  void Zero();
  void Norm();

  double GetProbability(const std::vector<double>& seq);
  double Decode(const std::vector<double>& seq, std::vector<int>* state);

  /* SampleFile: <size><dim><seq_size><seq_data>...<seq_size><seq_data>...
   *              int   int    int      double  seq_data_size = dim * seq_size
   *  size: numble of seq samples
   *  dim: dimension of feature
   *  seq_size: number of each sample's feature
   */
  void Train(const char* sample_filename);
  void Train(const std::vector<std::vector<double> >& data);
  void Init(const std::vector<std::vector<double> >& data);

  friend std::ostream& operator << (std::ostream& out, HMM& hmm);
  friend std::istream& operator >> (std::istream& in, HMM& hmm);

 private:
  void Allocate(int state, int dim = 1, int mix = 1);
  void Free();
  double LogProb(double p);

  int state_num_;
  int dim_num_;
  int max_iter_num_;

  double end_error_;
  std::vector<GMM*> state_model_;
  std::vector<double> state_init_;
  std::vector<double> state_tran_;
};

}  // namespace base

#endif
