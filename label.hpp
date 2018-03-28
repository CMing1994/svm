/*   Support Vector Machine Library Wrappers
 *   Copyright (C) 2018  Jonas Greitemann
 *  
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program, see the file entitled "LICENCE" in the
 *   repository's root directory, or see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>
#include <stdexcept>

#define SVM_LABEL_BEGIN(LABELNAME, LABELCOUNT)                          \
namespace LABELNAME {                                                   \
    const char * NAMES[3];                                              \
    struct label {                                                      \
        static const size_t number_classes = (LABELCOUNT);              \
        label (short val) : val (val) {}                                \
        label (short val, const char * c_str) : val (val) {             \
            NAMES[val] = c_str;                                         \
        }                                                               \
        label (double x) : val (x + 0.5) {                              \
            if (x < 0 || x >= number_classes)                           \
                throw std::runtime_error("invalid label");              \
        }                                                               \
        operator double() const { return val; }                         \
        friend bool operator== (label lhs, label rhs) {                 \
            return lhs.val == rhs.val;                                  \
        }                                                               \
        friend std::ostream & operator<< (std::ostream & os, label l) { \
            os << NAMES[l.val];                                         \
        }                                                               \
    private:                                                            \
        const short val;                                                \
    };                                                                  \
    short i = 0;

#define SVM_LABEL_ADD(OPTIONNAME)                   \
    const label OPTIONNAME { i++, #OPTIONNAME };

#define SVM_LABEL_END() }
