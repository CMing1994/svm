#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "model_test.hpp"
#include "hyperplane_model.hpp"

#include <random>


TEST_CASE("hyperplane-builtin") {
    std::mt19937 rng(42);
    hyperplane_model trial_model(25, rng);
    model_test<svm::kernel::linear>(2500, 0.98, trial_model, rng);
}
