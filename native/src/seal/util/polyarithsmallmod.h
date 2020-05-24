// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "seal/modulus.h"
#include "seal/util/common.h"
#include "seal/util/pointer.h"
#include "seal/util/polycore.h"
#include "seal/util/uintarithsmallmod.h"
#include "seal/util/iterator.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace seal
{
    namespace util
    {
        inline void modulo_poly_coeffs(
            ConstCoeffIter poly, std::size_t coeff_count, const Modulus &modulus, CoeffIter result)
        {
#ifdef SEAL_DEBUG
            if (!poly && coeff_count > 0)
            {
                throw std::invalid_argument("poly");
            }
            if (!result && coeff_count > 0)
            {
                throw std::invalid_argument("result");
            }
            if (modulus.is_zero())
            {
                throw std::invalid_argument("modulus");
            }
#endif
            std::transform(*poly, *poly + coeff_count, *result, [&](auto coeff) {
                uint64_t temp[2]{ coeff, 0 };
                return barrett_reduce_128(temp, modulus);
            });
        }

        inline void modulo_poly_coeffs(
            ConstRNSIter poly, std::size_t coeff_modulus_size, ModulusIter modulus, RNSIter result)
        {
            SEAL_ITERATE(iter_tuple(poly, modulus, result), coeff_modulus_size, [&](auto I) {
                modulo_poly_coeffs(get<0>(I), poly.poly_modulus_degree(), *get<1>(I), get<2>(I)); 
            });
        }

        inline void modulo_poly_coeffs_63(
            ConstCoeffIter poly, std::size_t coeff_count, const Modulus &modulus, CoeffIter result)
        {
#ifdef SEAL_DEBUG
            if (!poly && coeff_count > 0)
            {
                throw std::invalid_argument("poly");
            }
            if (!result && coeff_count > 0)
            {
                throw std::invalid_argument("result");
            }
            if (modulus.is_zero())
            {
                throw std::invalid_argument("modulus");
            }
#endif
            // This function is the fastest for reducing polynomial coefficients,
            // but requires that the input coefficients are at most 63 bits, unlike
            // modulo_poly_coeffs that allows also 64-bit coefficients.
            std::transform(
                *poly, *poly + coeff_count, *result, [&](auto coeff) { return barrett_reduce_63(coeff, modulus); });
        }

        inline void modulo_poly_coeffs_63(
            ConstRNSIter poly, std::size_t coeff_modulus_size, ModulusIter modulus, RNSIter result)
        {
            SEAL_ITERATE(iter_tuple(poly, modulus, result), coeff_modulus_size, [&](auto I) {
                modulo_poly_coeffs_63(get<0>(I), poly.poly_modulus_degree(), *get<1>(I), get<2>(I)); 
            });
        }

        inline void negate_poly_coeffmod(
            ConstCoeffIter poly, std::size_t coeff_count, const Modulus &modulus, CoeffIter result)
        {
#ifdef SEAL_DEBUG
            if (!poly && coeff_count > 0)
            {
                throw std::invalid_argument("poly");
            }
            if (modulus.is_zero())
            {
                throw std::invalid_argument("modulus");
            }
            if (!result && coeff_count > 0)
            {
                throw std::invalid_argument("result");
            }
#endif
            const uint64_t modulus_value = modulus.value();
            SEAL_ITERATE(iter_tuple(poly, result), coeff_count, [&](auto I) {
                auto coeff = *get<0>(I);
#ifdef SEAL_DEBUG
                if (coeff >= modulus_value)
                {
                    throw std::out_of_range("poly");
                }
#endif
                std::int64_t non_zero = (coeff != 0);
                *get<1>(I) = (modulus_value - coeff) & static_cast<std::uint64_t>(-non_zero);
            });
        }

        inline void negate_poly_coeffmod(
            ConstRNSIter poly, std::size_t coeff_modulus_size, ModulusIter modulus, RNSIter result)
        {
            SEAL_ITERATE(iter_tuple(poly, modulus, result), coeff_modulus_size, [&](auto I) {
                negate_poly_coeffmod(get<0>(I), poly.poly_modulus_degree(), *get<1>(I), get<2>(I)); 
            });
        }

        inline void add_poly_coeffmod(
            ConstCoeffIter operand1, ConstCoeffIter operand2, std::size_t coeff_count,
            const Modulus &modulus, CoeffIter result)
        {
#ifdef SEAL_DEBUG
            if (!operand1 && coeff_count > 0)
            {
                throw std::invalid_argument("operand1");
            }
            if (!operand2 && coeff_count > 0)
            {
                throw std::invalid_argument("operand2");
            }
            if (modulus.is_zero())
            {
                throw std::invalid_argument("modulus");
            }
            if (!result && coeff_count > 0)
            {
                throw std::invalid_argument("result");
            }
#endif
            const uint64_t modulus_value = modulus.value();
            SEAL_ITERATE(iter_tuple(operand1, operand2, result), coeff_count, [&](auto I) {
#ifdef SEAL_DEBUG
                if (*get<0>(I) >= modulus_value)
                {
                    throw std::invalid_argument("operand1");
                }
                if (*get<1>(I) >= modulus_value)
                {
                    throw std::invalid_argument("operand2");
                }
#endif
                std::uint64_t sum = *get<0>(I) + *get<1>(I);
                *get<2>(I) = sum - (modulus_value &
                                 static_cast<std::uint64_t>(-static_cast<std::int64_t>(sum >= modulus_value)));
            });
        }

        inline void add_poly_coeffmod(
            ConstRNSIter operand1, ConstRNSIter operand2, std::size_t coeff_modulus_size, ModulusIter modulus, RNSIter result)
        {
            SEAL_ITERATE(iter_tuple(operand1, operand2, modulus, result), coeff_modulus_size, [&](auto I) {
                add_poly_coeffmod(get<0>(I), get<1>(I), operand1.poly_modulus_degree(), *get<2>(I), get<3>(I)); 
            });
        }

        inline void sub_poly_coeffmod(
            const std::uint64_t *operand1, const std::uint64_t *operand2, std::size_t coeff_count,
            const Modulus &modulus, std::uint64_t *result)
        {
#ifdef SEAL_DEBUG
            if (operand1 == nullptr && coeff_count > 0)
            {
                throw std::invalid_argument("operand1");
            }
            if (operand2 == nullptr && coeff_count > 0)
            {
                throw std::invalid_argument("operand2");
            }
            if (modulus.is_zero())
            {
                throw std::invalid_argument("modulus");
            }
            if (result == nullptr && coeff_count > 0)
            {
                throw std::invalid_argument("result");
            }
#endif
            const uint64_t modulus_value = modulus.value();
            for (; coeff_count--; result++, operand1++, operand2++)
            {
#ifdef SEAL_DEBUG
                if (*operand1 >= modulus_value)
                {
                    throw std::out_of_range("operand1");
                }
                if (*operand2 >= modulus_value)
                {
                    throw std::out_of_range("operand2");
                }
#endif
                unsigned long long temp_result;
                std::int64_t borrow = sub_uint64(*operand1, *operand2, &temp_result);
                *result = temp_result + (modulus_value & static_cast<std::uint64_t>(-borrow));
            }
        }

        void multiply_poly_scalar_coeffmod(
            const std::uint64_t *poly, std::size_t coeff_count, std::uint64_t scalar, const Modulus &modulus,
            std::uint64_t *result);

        void multiply_poly_coeffmod(
            const std::uint64_t *operand1, std::size_t operand1_coeff_count, const std::uint64_t *operand2,
            std::size_t operand2_coeff_count, const Modulus &modulus, std::size_t result_coeff_count,
            std::uint64_t *result);

        void multiply_poly_coeffmod(
            const std::uint64_t *operand1, const std::uint64_t *operand2, std::size_t coeff_count,
            const Modulus &modulus, std::uint64_t *result);

        inline void multiply_truncate_poly_coeffmod(
            const std::uint64_t *operand1, const std::uint64_t *operand2, std::size_t coeff_count,
            const Modulus &modulus, std::uint64_t *result)
        {
            multiply_poly_coeffmod(operand1, coeff_count, operand2, coeff_count, modulus, coeff_count, result);
        }

        void divide_poly_coeffmod_inplace(
            std::uint64_t *numerator, const std::uint64_t *denominator, std::size_t coeff_count, const Modulus &modulus,
            std::uint64_t *quotient);

        inline void divide_poly_coeffmod(
            const std::uint64_t *numerator, const std::uint64_t *denominator, std::size_t coeff_count,
            const Modulus &modulus, std::uint64_t *quotient, std::uint64_t *remainder)
        {
            set_uint(numerator, coeff_count, remainder);
            divide_poly_coeffmod_inplace(remainder, denominator, coeff_count, modulus, quotient);
        }

        void dyadic_product_coeffmod(
            const std::uint64_t *operand1, const std::uint64_t *operand2, std::size_t coeff_count,
            const Modulus &modulus, std::uint64_t *result);

        std::uint64_t poly_infty_norm_coeffmod(
            const std::uint64_t *operand, std::size_t coeff_count, const Modulus &modulus);

        bool try_invert_poly_coeffmod(
            const std::uint64_t *operand, const std::uint64_t *poly_modulus, std::size_t coeff_count,
            const Modulus &modulus, std::uint64_t *result, MemoryPool &pool);

        void negacyclic_shift_poly_coeffmod(
            const std::uint64_t *operand, std::size_t coeff_count, std::size_t shift, const Modulus &modulus,
            std::uint64_t *result);

        inline void negacyclic_multiply_poly_mono_coeffmod(
            const std::uint64_t *operand, std::size_t coeff_count, std::uint64_t mono_coeff, std::size_t mono_exponent,
            const Modulus &modulus, std::uint64_t *result, MemoryPool &pool)
        {
            auto temp(util::allocate_uint(coeff_count, pool));
            multiply_poly_scalar_coeffmod(operand, coeff_count, mono_coeff, modulus, temp.get());
            negacyclic_shift_poly_coeffmod(temp.get(), coeff_count, mono_exponent, modulus, result);
        }
    } // namespace util
} // namespace seal
