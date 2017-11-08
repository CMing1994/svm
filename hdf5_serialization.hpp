#pragma once

#include "svm.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <alps/hdf5/archive.hpp>
#include <alps/hdf5/vector.hpp>
#include <alps/hdf5/multi_array.hpp>


namespace svm {

    struct hdf5_tag;

    template <typename Model>
    struct serializer<hdf5_tag, Model> {

        serializer (Model & m) : model_(m) {}

        void save (std::string const& filename) const {
            alps::hdf5::archive ar(filename, "w");
            save(ar);
        }

        void save (alps::hdf5::archive & ar) const {
            save_problem(model_.prob, ar);

            const svm_parameter& param = model_.m->param;

            ar["model/param/svm_type"] << param.svm_type;
            ar["model/param/kernel_type"] << param.kernel_type;

            if (param.kernel_type == POLY)
                ar["model/param/degree"] << param.degree;

            if(param.kernel_type == POLY || param.kernel_type == RBF || param.kernel_type == SIGMOID)
                ar["model/param/gamma"] << param.gamma;

            if(param.kernel_type == POLY || param.kernel_type == SIGMOID)
                ar["model/param/coef0"] << param.coef0;

            int nr_class = model_.m->nr_class;
            int nr_sum = nr_class*(nr_class-1)/2;
            int l = model_.m->l;

            {
                std::vector<double> rho(model_.m->rho, model_.m->rho + nr_sum);
                ar["model/rho"] << rho;
            }

            if (model_.m->label) {
                std::vector<int> label(model_.m->label, model_.m->label + nr_class);
                ar["model/label"] << label;
            }

            if (model_.m->probA) // regression has probA only
            {
                std::vector<double> probA(model_.m->probA,
                                          model_.m->probA + nr_sum);
                ar["model/probA"] << probA;
            }
            if(model_.m->probB)
            {
                std::vector<double> probB(model_.m->probB,
                                          model_.m->probB + nr_sum);
                ar["model/probB"] << probB;
            }

            if(model_.m->nSV)
            {
                std::vector<int> nSV(model_.m->nSV, model_.m->nSV + nr_class);
                ar["model/nSV"] << nSV;
            }

            const double * const *sv_coef = model_.m->sv_coef;
            const svm_node * const *SV = model_.m->SV;

            boost::multi_array<double,2> sv_coefm(boost::extents[l][nr_class-1]);
            boost::multi_array<double,2> SVm(boost::extents[l][model_.prob.dim()]);
            for (int i = 0; i < l; ++i) {
                for (int j = 0; j < nr_class - 1; ++j)
                    sv_coefm[i][j] = sv_coef[j][i];

                if (param.kernel_type == PRECOMPUTED)
                    SVm[i][0] = SV[i]->value;
                else {
                    svm::data_view v(SV[i]);
                    auto it = v.begin();
                    for (int j = 1; it != v.end(); ++j, ++it) {
                        SVm[i][j-1] = *it;
                    }
                }
            }
            ar["model/sv_coef"] << sv_coefm;
            ar["model/SV"] << SVm;
        }

        void load (std::string const& filename) {
            alps::hdf5::archive ar(filename, "r");
            load(ar);
        }

        void load (alps::hdf5::archive & ar) {
            model_.prob = load_problem<typename Model::problem_t>(ar);

            if (model_.m)
                svm_free_and_destroy_model(&model_.m);

            model_.m = (struct svm_model *)malloc(sizeof(struct svm_model));

            struct svm_parameter& param = model_.m->param;
            // parameters for training only won't be assigned, but arrays are
            // assigned as NULL for safety
            param.nr_weight = 0;
            param.weight_label = NULL;
            param.weight = NULL;

            ar["model/param/svm_type"] >> param.svm_type;
            ar["model/param/kernel_type"] >> param.kernel_type;
            if (param.kernel_type == POLY)
                ar["model/param/degree"] >> param.degree;
            if(param.kernel_type == POLY || param.kernel_type == RBF || param.kernel_type == SIGMOID)
                ar["model/param/gamma"] >> param.gamma;
            if(param.kernel_type == POLY || param.kernel_type == SIGMOID)
                ar["model/param/coef0"] >> param.coef0;

            std::vector<double> rho;
            ar["model/rho"] >> rho;

            std::vector<int> label;
            ar["model/label"] >> label;

            int nr_class = label.size();
            int nr_sum = rho.size();

            if (nr_sum != nr_class * (nr_class-1) / 2)
                throw std::runtime_error("inconsistent data length");
            model_.m->nr_class = nr_class;

            model_.m->rho = (double *)malloc(sizeof(double) * nr_sum);
            std::copy(rho.begin(), rho.end(), model_.m->rho);

            model_.m->label = (int *)malloc(sizeof(int) * nr_class);
            std::copy(label.begin(), label.end(), model_.m->label);

            model_.m->sv_indices = nullptr;
            if (ar.is_data("model/probA")) // regression has probA only
            {
                std::vector<double> probA;
                ar["model/probA"] >> probA;
                if (probA.size() != nr_sum)
                    throw std::runtime_error("inconsistent data length");
                model_.m->probA = (double *)malloc(sizeof(double) * nr_sum);
                std::copy(probA.begin(), probA.end(), model_.m->probA);
            } else
                model_.m->probA = nullptr;
            if (ar.is_data("model/probB"))
            {
                std::vector<double> probB;
                ar["model/probB"] >> probB;
                if (probB.size() != nr_sum)
                    throw std::runtime_error("inconsistent data length");
                model_.m->probB = (double *)malloc(sizeof(double) * nr_sum);
                std::copy(probB.begin(), probB.end(), model_.m->probB);
            } else
                model_.m->probB = nullptr;

            std::vector<int> nSV;
            ar["model/nSV"] >> nSV;
            if (nSV.size() != nr_class)
                throw std::runtime_error("inconsistent data length");
            model_.m->nSV = (int *)malloc(sizeof(int) * nr_class);
            std::copy(nSV.begin(), nSV.end(), model_.m->nSV);

            boost::multi_array<double,2> sv_coefm;
            ar["model/sv_coef"] >> sv_coefm;
            boost::multi_array<double,2> SVm;
            ar["model/SV"] >> SVm;

            int l = sv_coefm.shape()[0];
            if (sv_coefm.shape()[1] != nr_class-1)
                throw std::runtime_error("inconsistent data length");
            if (SVm.shape()[0] != l)
                throw std::runtime_error("inconsistent data length");
            if (SVm.shape()[1] != model_.prob.dim())
                throw std::runtime_error("inconsistent data length");
            model_.m->l = l;

            model_.m->sv_coef = (double **)malloc(sizeof(double *) * (nr_class-1));
            for (int j = 0; j < nr_class-1; ++j) {
                model_.m->sv_coef[j] = (double *)malloc(sizeof(double) * l);
                for (int i = 0; i < l; ++i) {
                    model_.m->sv_coef[j][i] = sv_coefm[i][j];
                }
            }

            model_.m->SV = (struct svm_node **)malloc(sizeof(struct svm_node *) * l);
            size_t start_index = Model::problem_t::is_precomputed ? 0 : 1;
            for (int i = 0; i < l; ++i) {
                svm::dataset ds(SVm[i].begin(), SVm[i].end(), start_index);
                model_.m->SV[i] = (struct svm_node *)malloc(sizeof(struct svm_node) * ds.data().size());
                std::copy(ds.data().begin(), ds.data().end(), model_.m->SV[i]);
            }

            model_.m->free_sv = 1;
            model_.params_ = typename Model::parameters_t(model_.m->param);
        }

    private:

        template <class Problem, typename = typename std::enable_if<Problem::is_precomputed>::type, bool dummy = false>
        void save_problem (Problem const& prob, alps::hdf5::archive & ar) const {
            using input_t = typename Problem::input_container_type;

            ar["prob/dim"] << prob.dim();

            boost::multi_array<double, 2> orig_data(boost::extents[prob.size()][prob.dim()]);
            std::vector<double> labels(prob.size());

            for (size_t i = 0; i < prob.size(); ++i) {
                auto p = prob[i];
                input_t const& xs = p.first;
                std::copy(xs.begin(), xs.end(), orig_data[i].begin());
                labels[i] = p.second;
            }

            ar["prob/orig_data"] << orig_data;
            ar["prob/labels"] << labels;
        }

        template <class Problem, typename = typename std::enable_if<Problem::is_precomputed>::type, bool dummy = false>
        Problem load_problem (alps::hdf5::archive & ar) const {
            using input_t = typename Problem::input_container_type;

            size_t dim;
            ar["prob/dim"] >> dim;
            Problem prob(dim);

            boost::multi_array<double, 2> orig_data;
            std::vector<double> labels;
            ar["prob/orig_data"] >> orig_data;
            ar["prob/labels"] >> labels;

            if (orig_data.shape()[0] != labels.size())
                throw std::runtime_error("inconsistent data length");
            if (orig_data.shape()[1] != dim)
                throw std::runtime_error("inconsistent data length");

            for (size_t i = 0; i < labels.size(); ++i)
                prob.add_sample(input_t(orig_data[i].begin(),
                                        orig_data[i].end()),
                                labels[i]);

            return prob;
        }

        template <class Problem, typename = typename std::enable_if<!Problem::is_precomputed>::type>
        void save_problem(Problem const& prob, alps::hdf5::archive & ar) const {
            ar["prob/dim"] << prob.dim();
        }

        template <class Problem, typename = typename std::enable_if<!Problem::is_precomputed>::type>
        Problem load_problem (alps::hdf5::archive & ar) const {
            size_t dim;
            ar["prob/dim"] >> dim;
            Problem prob(dim);
            return prob;
        }

        Model & model_;

    };

}
