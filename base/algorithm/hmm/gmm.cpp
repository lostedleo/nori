/*************************************************************************
  > File Name:    gmm.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Tue 01 Sep 2020 06:38:03 PM CST
 ************************************************************************/

#include <iostream>
#include <math.h>
#include <assert.h>
#include <string>
#include <stdio.h>

#include  "algorithm/hmm/kmeans.h"
#include  "algorithm/hmm/gmm.h"

namespace base {

GMM::GMM(int dim_num /*=1*/, int mix_num/*=1*/) {
  dim_num_ = dim_num;
  mix_num_ = mix_num;
  max_iter_num_ = 100;
  end_error_ = 0.001;

  Allocate();
  for (int i = 0; i < mix_num_; ++i) {
    priors_[i] = 1.0 / mix_num_;

    for (int j = 0; j < dim_num_; ++j) {
      vars(i)[j] = 1;
    }
  }
}

GMM::~GMM() {
  Free();
}

void GMM::Copy(GMM* gmm) {
  assert(mix_num_ == gmm->mix_num_ && dim_num_ == gmm->dim_num_);
  memcpy(priors_.data(), gmm->priors_.data(), sizeof(double) * mix_num_);
  memcpy(means_.data(), gmm->means_.data(), sizeof(double) * mix_num_ * dim_num_);
  memcpy(vars_.data(), gmm->vars_.data(), sizeof(double) * mix_num_ * dim_num_);
  memcpy(min_vars_.data(), gmm->min_vars_.data(), sizeof(double) * dim_num_);
}

double GMM::GetProbability(const double* sample) {
  double p = 0;
  for (int i = 0; i < mix_num_; ++i) {
    p += priors_[i] * GetProbability(sample, i);
  }
  return p;
}

void GMM::Init(const char* sample_filename) {
  const double MIN_VAR = 1E-10;

  KMeans kmeans = KMeans(dim_num_, mix_num_);
  kmeans.SetInitMode(KMeans::InitUniform);
  std::string label_filename = "gmm_init.tmp";
  kmeans.Cluster(sample_filename, label_filename.c_str());

  std::vector<int> counts(mix_num_, 0);
  std::vector<double> over_means(dim_num_, 0);

  memcpy(means_.data(), kmeans.means(0), sizeof(double) * mix_num_ * dim_num_);
  memset(vars_.data(), 0, sizeof(double) * mix_num_ * dim_num_);
  memset(priors_.data(), 0, sizeof(double) * mix_num_);

  // Open the sample file and label file to initialize the model
  std::ifstream sample_file(sample_filename, std::ios_base::binary);
  assert(sample_file);

  std::ifstream label_file(label_filename.c_str(), std::ios_base::binary);
  assert(label_file);

  int num = 0;
  sample_file.read((char*)&num, sizeof(int));  // NOLINT
  sample_file.seekg(2 * sizeof(int), std::ios_base::beg);
  label_file.seekg(sizeof(int), std::ios_base::beg);

  std::vector<double> sample(dim_num_, 0);
  int label = -1;

  for (int i = 0; i < num; ++i) {
    sample_file.read((char*)sample.data(), sizeof(double) * dim_num_);  // NOLINT
    label_file.read((char*)&label, sizeof(int));  // NOLINT

    // Count each Gaussian
    counts[label]++;
    double* m = kmeans.means(label);
    for (int k = 0; k < dim_num_; ++k) {
      vars(label)[k] += (sample[k] - m[k]) * (sample[k] * m[k]);
    }

    // Count the overall mean and variance
    for (int k = 0; k < dim_num_; ++k) {
      over_means[k] += sample[k];
      min_vars_[k] += sample[k] * sample[k];
    }
  }

  // Compute teh overall variance (* 0.01) as the minimum variance
  for (int k = 0; k < dim_num_; ++k) {
    over_means[k] /= num;
    min_vars_[k] = std::max(MIN_VAR, 0.01 * (min_vars_[k] / num - over_means[k] * over_means[k]));
  }

  // Initialize each Gaussian
  for (int i = 0; i < mix_num_; ++i) {
    priors_[i] = 1.0 * counts[i] / num;
    if (priors_[i] > 0) {
      for (int j = 0; j < dim_num_; ++j) {
        vars(i)[j] = vars(i)[j] / counts[i];

        if (vars(i)[j] < min_vars_[j]) {
          vars(i)[j] = min_vars_[j];
        }
      }
    } else {
      memcpy(vars(i), min_vars_.data(), sizeof(double) * dim_num_);
      std::cout << "[WARNING] Gaussian " << i << " of GMM is not used!\n";
    }
  }

  sample_file.close();
  label_file.close();
  remove(label_filename.c_str());
}

void GMM::Train(const char* sample_filename) {
  std::ifstream sample_file(sample_filename, std::ios_base::binary);
  assert(sample_file);

  int num = 0;
  sample_file.seekg(0, std::ios_base::beg);
  sample_file.read((char*)&num, sizeof(int));  // NOLINT

  std::vector<double> data(num * dim_num_, 0);
  sample_file.seekg(2 * sizeof(int), std::ios_base::beg);
  sample_file.read((char*)data.data(), sizeof(double) * num * dim_num_);  // NOLINT

  Train(data.data(), num);
  sample_file.close();
}

void GMM::Init(double* data, int num) {
  const double MIN_VAR = 1E-10;

  KMeans kmeans = KMeans(dim_num_, mix_num_);
  kmeans.SetInitMode(KMeans::InitUniform);
  std::vector<int> labels(num, 0);
  kmeans.Cluster(data, num, labels.data());

  std::vector<int> counts(mix_num_, 0);
  std::vector<double> over_means(dim_num_, 0);

  memcpy(means_.data(), kmeans.means(0), sizeof(double) * mix_num_ * dim_num_);
  memset(priors_.data(), 0, sizeof(double) * mix_num_);
  memset(vars_.data(), 0, sizeof(double) * mix_num_ * dim_num_);

  double* sample = NULL;
  int label = -1;
  for (int i = 0; i < num; ++i) {
    label = labels[i];
    // Count each Gaussian
    counts[label]++;
    double* m = kmeans.means(label);
    sample = data + i * dim_num_;
    for (int j = 0; j < dim_num_; ++j) {
      vars(label)[j] += (sample[j] - m[j]) * (sample[j] * m[j]);
    }

    // Count the overall mean and variance
    for (int j = 0; j < dim_num_; ++j) {
      over_means[j] += sample[j];
      min_vars_[j] += sample[j] * sample[j];
    }
  }

  // Compute teh overall variance (* 0.01) as the minimum variance
  for (int k = 0; k < dim_num_; ++k) {
    over_means[k] /= num;
    min_vars_[k] = std::max(MIN_VAR, 0.01 * (min_vars_[k] / num - over_means[k] * over_means[k]));
  }

  // Initialize each Gaussian
  for (int i = 0; i < mix_num_; ++i) {
    priors_[i] = 1.0 * counts[i] / num;
    if (priors_[i] > 0) {
      for (int j = 0; j < dim_num_; ++j) {
        vars(i)[j] = vars(i)[j] / counts[i];

        if (vars(i)[j] < min_vars_[j]) {
          vars(i)[j] = min_vars_[j];
        }
      }
    } else {
      memcpy(vars(i), min_vars_.data(), sizeof(double) * dim_num_);
      std::cout << "[WARNING] Gaussian " << i << " of GMM is not used!\n";
    }
  }
}

void GMM::Train(double* data, int num) {
  Init(data, num);

  bool loop = true;
  double iter_num = 0, last = 0, curr = 0;
  int unchanged = 0;
  double* sample = NULL;
  std::vector<double> next_priors(mix_num_, 0);
  std::vector<double> next_vars(mix_num_ * dim_num_, 0);
  std::vector<double> next_means(mix_num_ * dim_num_, 0);

  // Reestimation
  while (loop) {
    last = curr;
    curr = 0;
    // Predict
    for (int i = 0; i < num; ++i) {
      sample = data + i * dim_num_;
      double p = GetProbability(sample);

      for (int j = 0; j < mix_num_; ++j) {
        double pj = GetProbability(sample, j) * priors_[j] / p;
        next_priors[j] += pj;

        for (int k = 0; k < dim_num_; ++k) {
          next_means[j * dim_num_ + k] += pj * sample[k];
          next_vars[j * dim_num_ + k] += pj * sample[k] * sample[k];
        }
      }

      curr += (p > 1E-20) ? log10(p) : -20;
    }
    curr /= num;

    // Reestimation: generate new priors, means and variances
    for (int i = 0; i < mix_num_; ++i) {
      priors_[i] = next_priors[i] / num;
      if (priors_[i] > 0) {
        for (int j = 0; j < dim_num_; ++j) {
          means(i)[j] = next_means[i * dim_num_ + j] / next_priors[i];
          vars(i)[j] = next_vars[i * dim_num_ + j] / next_priors[i] - means(i)[j] * means(i)[j];
          if (vars(i)[j] < min_vars_[j]) {
            vars(i)[j] = min_vars_[j];
          }
        }
      }
    }

    // Terminal conditions
    iter_num++;
    if (fabs(curr - last) < end_error_ * fabs(last)) {
      unchanged++;
    }
    if (iter_num >= max_iter_num_ || unchanged >=3) {
      loop = false;
    }
    memset(next_priors.data(), 0, sizeof(double) * mix_num_);
    memset(next_vars.data(), 0, sizeof(double) * mix_num_ * dim_num_);
    memset(next_means.data(), 0, sizeof(double) * mix_num_ * dim_num_);
  }
}

std::ostream& operator << (std::ostream& out, GMM& gmm) {
  out << "<GMM>" << std::endl;
  out << "<DimNum> " << gmm.dim_num_ << " </DimNum>" << std::endl;
  out << "<MixNum> " << gmm.mix_num_ << " </MixNum>" << std::endl;

  out << "<Prior> ";
  for (int i = 0; i < gmm.mix_num(); ++i) {
    out << gmm.prior(i) << " ";
  }
  out << "</Prior>" << std::endl;

  out << "<Mean>" << std::endl;
  for (int i = 0; i < gmm.mix_num(); ++i) {
    for (int j = 0; j < gmm.dim_num(); ++j) {
      out << gmm.means(i)[j] << " ";
    }
    out << std::endl;
  }
  out << "</Mean>" << std::endl;

  out << "<Variance>" << std::endl;
  for (int i = 0; i < gmm.mix_num(); ++i) {
    for (int j = 0; j < gmm.dim_num(); ++j) {
      out << gmm.vars(i)[j] << " ";
    }
    out << std::endl;
  }
  out << "</Variance>" << std::endl;

  out << "</GMM>" << std::endl;
  return out;
}

std::istream& operator >> (std::istream& in, GMM& gmm) {
  char label[50];
  in >> label;  // "<GMM>"
  assert(strcmp(label, "<GMM>") == 0);

  gmm.Free();
  in >> label >> gmm.dim_num_ >> label;  // "<DimNum>"
  in >> label >> gmm.mix_num_ >> label;  // "<MixNum>"
  gmm.Allocate();

  in >> label;  // "<Prior>"
  for (int i = 0; i < gmm.mix_num(); ++i) {
    in >> gmm.priors_[i];
  }
  in >> label;

  in >> label;  // "<Mean>"
  for (int i = 0; i < gmm.mix_num(); ++i) {
    for (int j = 0; j < gmm.dim_num(); ++j) {
      in >> gmm.means(i)[j];
    }
  }
  in >> label;

  in >> label;  // "<Variance>"
  for (int i = 0; i < gmm.mix_num(); ++i) {
    for (int j = 0; j < gmm.dim_num(); ++j) {
      in >> gmm.vars(i)[j];
    }
  }
  in >> label;

  in >> label;  // "</GMM>"
  return in;
}

double GMM::GetProbability(const double* x, int j) {
  double p = 1;
  for (int i = 0; i < dim_num_; ++i) {
    p *= 1 / sqrt(2 * 3.14159 * vars(j)[i]);
    p *= exp(-0.5 * (x[i] - means(j)[i]) * (x[i] - means(j)[i]) / vars(j)[i]);
  }
  return p;
}

void GMM::Allocate() {
  priors_.resize(mix_num_);
  means_.resize(mix_num_ * dim_num_);
  vars_.resize(mix_num_ * dim_num_);
  min_vars_.resize(dim_num_);
}

void GMM::Free() {
  priors_.clear();
  means_.clear();
  vars_.clear();
  min_vars_.clear();
}

}  // namespace base
