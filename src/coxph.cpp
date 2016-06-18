//-----------------------------------
//
// File: coxph.cpp
//
// Description: Cox partial hazards model.
//
//-----------------------------------

//-----------------------------------
// Includes
//-----------------------------------
#include "coxph.h"
#include "censored_cox_state.h"
#include "counting_cox_state.h"
#include <Rinternals.h>
#include <math.h>

namespace {
int GetTiesMethod(const std::string& selection) {
  if (selection == "efron") {
    return 1;
  } else if (selection == "breslow") {
    return 0;
  }

  throw gbm_exception::InvalidArgument("unknown tie-handling method");
}
}

//----------------------------------------
// Function Members - Private
//----------------------------------------
CCoxPH::CCoxPH(double* stats, int* sorted_end, int* sorted_start, int* strats,
               bool is_startstop, int tiesmethod, double priorcoeff)
    : kStartStopCase_(is_startstop),
      sortedendtimes_(sorted_end),
      sortedstarttimes_(sorted_start),
      strata(strats),
      kPriorCoeffVariation_(priorcoeff) {
  status_ = stats;
  tiedtimesmethod_ = tiesmethod;

  // Set up which methods CoxPh will use
  if (kStartStopCase_) {
    coxstate_methods_.reset(new CountingCoxState(this));
  } else {
    coxstate_methods_.reset(new CensoredCoxState(this));
  }
}

//----------------------------------------
// Function Members - Public
//----------------------------------------
CDistribution* CCoxPH::Create(DataDistParams& distparams) {
  // Initialize variables to pass to constructor
  double* stat = 0;
  int* sortedst = 0;
  int* sortedend = 0;
  bool isstartstop = false;
  int tiesmethod = GetTiesMethod(Rcpp::as<string>(distparams.misc[0]));

  // Set up strata
  Rcpp::IntegerVector strats(distparams.strata);

  // Check if start/stop case or not
  Rcpp::IntegerMatrix sortMatrix(distparams.sorted);
  if (distparams.response.ncol() > 2) {
    isstartstop = true;
    stat = distparams.response(Rcpp::_, 2).begin();
    sortedend = sortMatrix(Rcpp::_, 1).begin();
    sortedst = sortMatrix(Rcpp::_, 0).begin();

    return new CCoxPH(stat, sortedend, sortedst, strats.begin(), isstartstop,
                      tiesmethod, distparams.prior_coefficient_variation);
  }

  // If not start/stop
  stat = distparams.response(Rcpp::_, 1).begin();
  sortedend = sortMatrix(Rcpp::_, 0).begin();

  return new CCoxPH(stat, sortedend, sortedst, strats.begin(), isstartstop,
                    tiesmethod, distparams.prior_coefficient_variation);
}

void CCoxPH::ComputeWorkingResponse(const CDataset& kData, const Bag& kBag,
                                    const double* kFuncEstimate,
                                    std::vector<double>& residuals) {
  coxstate_methods_->ComputeWorkingResponse(kData, kBag, kFuncEstimate,
                                            residuals);
}

double CCoxPH::InitF(const CDataset& kData) { return 0.0; }

double CCoxPH::Deviance(const CDataset& kData, const Bag& kBag,
                        const double* kFuncEstimate) {
  // Set size and move to validation set if necessary
  unsigned long num_rows_in_set = kData.get_size_of_set();

  double returnvalue = 0.0;
  returnvalue =
      coxstate_methods_->Deviance(num_rows_in_set, kData, kBag, kFuncEstimate);

  return returnvalue;
}

void CCoxPH::FitBestConstant(const CDataset& kData, const Bag& kBag,
                             const double* kFuncEstimate,
                             unsigned long num_terminalnodes,
                             std::vector<double>& residuals, CCARTTree& tree) {
  coxstate_methods_->FitBestConstant(kData, kBag, kFuncEstimate,
                                     num_terminalnodes, residuals, tree);
}

double CCoxPH::BagImprovement(const CDataset& kData, const Bag& kBag,
                              const double* kFuncEstimate,
                              const double kShrinkage,
                              const std::vector<double>& kDeltaEstimate) {
  return coxstate_methods_->BagImprovement(kData, kBag, kFuncEstimate,
                                           kShrinkage, kDeltaEstimate);
}
