/*************************************************************************
  > File Name:    hmm.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Fri 04 Sep 2020 11:42:08 AM CST
 ************************************************************************/

#include <math.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <assert.h>
#include <string.h>

#include "algorithm/hmm/hmm.h"
#include "algorithm/hmm/gmm.h"

namespace base {

HMM::HMM(int state_num/*=1*/, int dim_num/*=1*/, int mix_num/*=1*/) {
  state_num_ = state_num;
  dim_num_ = dim_num;
  max_iter_num_ = 100;
  end_error_ = 0.001;

  Allocate(state_num, dim_num, mix_num);

  for (int i = 0; i < state_num_; ++i) {
    state_init_[i] = 1.0 / state_num_;
    for (int j = 0; j < state_num_ + 1; ++j) {
      state_tran_[i * state_num_ + j] = 1.0 / (state_num_ + 1);
    }
  }
}

HMM::~HMM() {
  Free();
}

GMM* HMM::GetStateModel(int index) {
  assert(index >= 0 && index < state_num_);
  return state_model_[index];
}

double HMM::GetStateInit(int index) {
  assert(index >= 0 && index < state_num_);
  return state_init_[index];
}

double HMM::GetStateFinal(int index) {
  assert(index >= 0 && index < state_num_);
  return state_tran_[index * state_num_ + state_num_];
}

double HMM::GetStateTrans(int i, int j) {
  assert(i >= 0 && i < state_num_ && j >=0 && j <= state_num_);
  return state_tran_[i * state_num_ + j];
}

void HMM::Zero() {
  for (int i = 0; i < state_num_; ++i) {
    state_init_[i] = 0;
    for (int j = 0; j < state_num_ + 1; ++j) {
      state_tran_[i * state_num_ + j] = 0;
    }
  }
}

void HMM::Norm() {
  double count = 0;
  for (int i = 0; i < state_num_; ++i) {
    count += state_init_[i];
  }
  for (int i = 0; i < state_num_; ++i) {
    state_init_[i] /= count;
  }

  for (int i = 0; i < state_num_; ++i) {
    count = 0;
    for (int j = 0; j < state_num_ + 1; ++j) {
      count += state_tran_[i * state_num_ + j];
    }
    if (count > 0) {
      for (int j = 0; j < state_num_ + 1; ++j) {
        state_tran_[i * state_num_ + j] /= count;
      }
    }
  }
}

double HMM::GetProbability(const std::vector<double>& seq) {
  std::vector<int> state;
  return Decode(seq, &state);
}

double HMM::Decode(const std::vector<double>& seq, std::vector<int>* state) {
  state->clear();

  // Viterbi
  int size = seq.size() / dim_num_;
  std::vector<double> last_logp(state_num_, 0);
  std::vector<double> curr_logp(state_num_, 0);
  std::vector<int> path(size * state_num_, 0);

  // Init
  for (int i = 0; i < state_num_; ++i) {
    curr_logp[i] = LogProb(state_init_[i])
      + LogProb(state_model_[i]->GetProbability(seq.data()));
    path[i] = -1;
  }

  // Recursion
  for (int i = 1; i < size; ++i) {
    last_logp.swap(curr_logp);

    for (int j = 0; j < state_num_; ++j) {
      curr_logp[j] = -1e308;
      for (int k = 0; k < state_num_; ++k) {
        double l = last_logp[k] + LogProb(state_tran_[k * state_num_ + j]);
        if (l > curr_logp[j]) {
          curr_logp[j] = l;
          path[i * state_num_ + j] = k;
        }
      }
      curr_logp[j] += LogProb(state_model_[j]->GetProbability(seq.data() + i * dim_num_));
    }
  }

  // Termination
  int final_state = 0;
  double prob = -1e308;
  for (int i = 0; i < state_num_; ++i) {
    if (curr_logp[i] > prob) {
      prob = curr_logp[i];
      final_state = i;
    }
  }

  // Decode
  state->push_back(final_state);
  int state_index = 0;
  for (int i = size - 2; i >= 0; --i) {
    state_index = path[(i + 1) * state_num_ + state->back()];
    state->push_back(state_index);
  }

  // Reverse the state list
  std::reverse(state->begin(), state->end());
  prob = exp(prob / size);
  return prob;
}

void HMM::Train(const char* sample_filename) {
  std::ifstream sample_file(sample_filename, std::ios_base::binary);
  assert(sample_file);

  int num = 0;
  int dim = 0;
  sample_file.read((char*)&num, sizeof(int));  // NOLINT
  sample_file.read((char*)&dim, sizeof(int));  // NOLINT

  std::vector<std::vector<double> > data;
  data.resize(num);
  int seq_size = 0;
  for (int i = 0; i < num; ++i) {
    sample_file.read((char*)&seq_size, sizeof(int));  // NOLINT
    data[i].resize(seq_size * dim);
    sample_file.read((char*)data[i].data(), sizeof(double) * seq_size * dim);  // NOLINT
  }

  Train(data);
  sample_file.close();
}

void HMM::Train(const std::vector<std::vector<double> >& data) {
  Init(data);

  std::vector<int> state_init_num(state_num_, 0);
  std::vector<int> state_tran_num(state_num_ * (state_num_ + 1), 0);

  int size = data.size();
  bool loop = true;
  double curr = 0, last = 0;
  int iter_num = 0, unchanged = 0;
  std::vector<int> state;
  std::vector<std::vector<double> > state_data;
  state_data.resize(state_num_);

  while (loop) {
    last = curr;
    curr = 0;

    int seq_size = 0;
    for (int i = 0; i < size; ++i) {
      curr += LogProb(Decode(data[i], &state));
      state_init_num[state[0]]++;

      seq_size = data[i].size() / dim_num_;
      for (int j = 0; j < seq_size; ++j) {
        for (int k = 0; k < dim_num_; ++k) {
          state_data[state[j]].push_back(data[i][j * dim_num_ + k]);
        }
        if (j > 0) {
          state_tran_num[state[j-1] * state_num_ + state[j]]++;
        }
      }
      // Final state
      state_tran_num[state[seq_size-1] * state_num_ + state_num_]++;
    }
    curr /= size;
    // Reestimation: state_model, state_init, state_tran
    int count = 0;
    for (int i = 0; i < state_num_; ++i) {
      int data_size = state_data[i].size() / dim_num_;
      if (data_size > state_model_[i]->mix_num() * 2) {
        state_model_[i]->Train(state_data[i].data(), data_size);
      }
      count += state_init_num[i];
    }
    for (int i = 0; i < state_num_; ++i) {
      state_init_[i] = 1.0 * state_init_num[i] / count;
    }
    for (int i = 0; i < state_num_; ++i) {
      count = 0;
      for (int j = 0; j < state_num_ + 1; ++j) {
        count += state_tran_num[i * (state_num_ + 1) + j];
      }
      if (count > 0) {
        for (int j = 0; j < state_num_ + 1; ++j) {
          state_tran_[i * state_num_ + j] = 1.0 * state_tran_num[i * state_num_ + j] / count;
        }
      }
    }

    // Terminal conditions
    iter_num++;
    unchanged = (curr - last < end_error_ * fabs(last)) ? (unchanged + 1) : 0;
    if (iter_num >= max_iter_num_ || unchanged >= 3) {
      loop = false;
    }

    memset(state_tran_num.data(), 0, sizeof(int) * state_num_ * (state_num_ + 1));
    memset(state_init_num.data(), 0, sizeof(int) * state_num_);
    for (int i = 0; i < state_num_; ++i) {
      state_data[i].clear();
    }

    // For debug
    /*std::cout << "Iter: " << iter_num << " unchanged: " << unchanged
      << " curr: " << curr << " last: " << last
      << ", average LogProbability: " << curr << std::endl;*/
  }
}

void HMM::Init(const std::vector<std::vector<double> >& data) {
  // Initialize probability
  for (int i = 0; i < state_num_; ++i) {
    // The initial probability
    if (i == 0) {
      state_init_[i] = 0.5;
    } else {
      state_init_[i] = 0.5 / float(state_num_ - 1);  // NOLINT
    }

    // The transition probability
    for (int j = 0; j < state_num_ + 1; ++j) {
      if ((i == j) || (j == i + 1)) {
        state_tran_[i * state_num_ + j] = 0.5;
      }
    }
  }

  std::vector<std::vector<double> > state_data;
  state_data.resize(state_num_);
  int size = data.size();
  int seq_size = 0;
  for (int i = 0; i < size; ++i) {
    assert(data[i].size() % dim_num_ == 0);
    seq_size = data[i].size() / dim_num_;

    double r = float(seq_size) / float(state_num_);  // NOLINT
    for (int j = 0; j < seq_size; ++j) {
      for (int k = 0; k < dim_num_; ++k) {
        state_data[int(j / r)].push_back(data[i][j * dim_num_ + k]);  // NOLINT
      }
    }
  }

  int num = 0;
  for (int i = 0; i < state_num_; ++i) {
    num = state_data[i].size() / dim_num_;
    state_model_[i]->Train(state_data[i].data(), num);
  }
}

std::ostream& operator << (std::ostream& out, HMM& hmm) {
  out << "<HMM>" << std::endl;
  out << "<StateNum> " << hmm.state_num_ << " </StateNum>" << std::endl;
  for (int i = 0; i < hmm.state_num_; ++i) {
    out << *hmm.state_model_[i];
  }

  out << "<Init> ";
  for (int i = 0; i < hmm.state_num_; ++i) {
    out << hmm.state_init_[i] << " ";
  }
  out << "</Init>" << std::endl;

  out << "<Tran>" << std::endl;
  for (int i = 0; i < hmm.state_num_; ++i) {
    for (int j = 0; j < hmm.state_num_ + 1; ++j) {
      out << hmm.GetStateTrans(i, j) << " ";
    }
    out << std::endl;
  }
  out << "</Tran>" << std::endl;
  out << "</HMM>" << std::endl;

  return out;
}

std::istream& operator >> (std::istream& in, HMM& hmm) {
  char label[20];
  in >> label;
  assert(strcmp(label, "<HMM>") == 0);

  hmm.Free();
  in >> label >> hmm.state_num_ >> label;
  hmm.Allocate(hmm.state_num_);

  for (int i = 0; i < hmm.state_num(); ++i) {
    in >> *hmm.state_model_[i];
  }

  // <Init>
  in >> label;
  for (int i = 0; i < hmm.state_num(); ++i) {
    in >> hmm.state_init_[i];
  }
  in >> label;

  // <Tran>
  for (int i = 0; i < hmm.state_num(); ++i) {
    for (int j = 0; j < hmm.state_num() + 1; ++j) {
      in >> hmm.state_tran_[i * hmm.state_num() + j];
    }
  }
  in >> label;

  // </HMM>
  in >> label;
  return in;
}

void HMM::Allocate(int state, int dim, int mix) {
  state_model_.resize(state);
  state_init_.resize(state);
  state_tran_.resize(state * (state + 1));

  for (int i = 0; i < state; ++i) {
    state_model_[i] = new GMM(dim, mix);
  }
}

void HMM::Free() {
  for (int i = 0; i < state_num_; ++i) {
    delete state_model_[i];
  }
  state_model_.clear();
}

double HMM::LogProb(double p) {
  return (p > 1e-20) ? log10(p) : -20;
}

}  // namespace base
