/*
    enoki/array_neon.h -- Packed SIMD array (ARM 64 bit NEON specialization)

    Enoki is a C++ template library that enables transparent vectorization
    of numerical kernels using SIMD instruction sets available on current
    processor architectures.

    Copyrighe (c) 2017 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a BSD-style
    license that can be found in the LICENSE file.
*/

#pragma once

#include "array_generic.h"

NAMESPACE_BEGIN(enoki)
NAMESPACE_BEGIN(detail)

template <> struct is_native<float, 4>  : std::true_type { };
template <> struct is_native<double, 2> : std::true_type { };
template <> struct is_native<float,  3> : std::true_type { };
template <typename T> struct is_native<T, 4, is_int32_t<T>> : std::true_type { };
template <typename T> struct is_native<T, 3, is_int32_t<T>> : std::true_type { };
template <typename T> struct is_native<T, 2, is_int64_t<T>> : std::true_type { };

NAMESPACE_END(detail)

ENOKI_INLINE uint64x2_t vmvnq_u64(uint64x2_t a) {
    return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(a)));
}

ENOKI_INLINE int64x2_t vmvnq_s64(int64x2_t a) {
    return vreinterpretq_s64_s32(vmvnq_s32(vreinterpretq_s32_s64(a)));
}

/// Partial overload of StaticArrayImpl using ARM NEON intrinsics (single precision)
template <bool Approx, typename Derived> struct ENOKI_MAY_ALIAS alignas(16)
    StaticArrayImpl<float, 4, Approx, RoundingMode::Default, Derived>
    : StaticArrayBase<float, 4, Approx, RoundingMode::Default, Derived> {
    ENOKI_NATIVE_ARRAY_CLASSIC(float, 4, Approx, float32x4_t)

    // -----------------------------------------------------------------------
    //! @{ \name Value constructors
    // -----------------------------------------------------------------------

    ENOKI_INLINE StaticArrayImpl(const Value &value) : m(vdupq_n_f32(value)) { }
    ENOKI_INLINE StaticArrayImpl(Value v0, Value v1, Value v2, Value v3)
        : m{v0, v1, v2, v3} { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Type converting constructors
    // -----------------------------------------------------------------------

    ENOKI_CONVERT(float) : m(a.derived().m) { }
    ENOKI_CONVERT(int32_t) : m(vcvtq_f32_s32(vreinterpretq_s32_u32(a.derived().m))) { }
    ENOKI_CONVERT(uint32_t) : m(vcvtq_f32_u32(a.derived().m)) { }
    ENOKI_CONVERT(half) : m(vcvt_f32_f16(vld1_f16((const __fp16 *) a.data()))) { }
    ENOKI_CONVERT(double) : m(vcvtx_high_f32_f64(vcvtx_f32_f64(low(a).m), high(a).m)) { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Reinterpreting constructors, mask converters
    // -----------------------------------------------------------------------

    ENOKI_REINTERPRET(float) : m(a.derived().m) { }
    ENOKI_REINTERPRET(int32_t) : m(vreinterpretq_f32_u32(a.derived().m)) { }
    ENOKI_REINTERPRET(uint32_t) : m(vreinterpretq_f32_u32(a.derived().m)) { }
    ENOKI_REINTERPRET(int64_t) : m(vreinterpretq_f32_u32(vcombine_u32(vmovn_u64(low(a).m), vmovn_u64(high(a).m)))) { }
    ENOKI_REINTERPRET(uint64_t) : m(vreinterpretq_f32_u32(vcombine_u32(vmovn_u64(low(a).m), vmovn_u64(high(a).m)))) { }
    ENOKI_REINTERPRET(double) : m(vreinterpretq_f32_u32(vcombine_u32(
        vmovn_u64(vreinterpretq_u64_f64(low(a).m)),
        vmovn_u64(vreinterpretq_u64_f64(high(a).m))))) { }
    ENOKI_REINTERPRET(bool) {
        m = vreinterpretq_f32_u32(uint32x4_t {
            reinterpret_array<uint32_t>(a.derived().coeff(0)),
            reinterpret_array<uint32_t>(a.derived().coeff(1)),
            reinterpret_array<uint32_t>(a.derived().coeff(2)),
            reinterpret_array<uint32_t>(a.derived().coeff(3))
        });
    }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Converting from/to half size vectors
    // -----------------------------------------------------------------------

    StaticArrayImpl(const Array1 &a1, const Array2 &a2)
        : m(_mm_setr_ps(a1.coeff(0), a1.coeff(1), a2.coeff(0), a2.coeff(1))) { }

    ENOKI_INLINE Array1 low_()  const { return Array1(coeff(0), coeff(1)); }
    ENOKI_INLINE Array2 high_() const { return Array2(coeff(2), coeff(3)); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Vertical operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Derived add_(Arg a) const { return vaddq_f32(m, a.m); }
    ENOKI_INLINE Derived sub_(Arg a) const { return vsubq_f32(m, a.m); }
    ENOKI_INLINE Derived mul_(Arg a) const { return vmulq_f32(m, a.m); }
    ENOKI_INLINE Derived div_(Arg a) const { return vdivq_f32(m, a.m); }

    ENOKI_INLINE Derived fmadd_(Arg b, Arg c) const { return vfmaq_f32(c.m, m, b.m); }
    ENOKI_INLINE Derived fnmadd_(Arg b, Arg c) const { return vfmsq_f32(c.m, m, b.m); }
    ENOKI_INLINE Derived fmsub_(Arg b, Arg c) const { return vfmaq_f32(vnegq_f32(c.m), m, b.m); }
    ENOKI_INLINE Derived fnmsub_(Arg b, Arg c) const { return vfmsq_f32(vnegq_f32(c.m), m, b.m); }

    ENOKI_INLINE Derived or_ (Arg a) const { return vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(m), vreinterpretq_s32_f32(a.m))); }
    ENOKI_INLINE Derived and_(Arg a) const { return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(m), vreinterpretq_s32_f32(a.m))); }
    ENOKI_INLINE Derived xor_(Arg a) const { return vreinterpretq_f32_s32(veorq_s32(vreinterpretq_s32_f32(m), vreinterpretq_s32_f32(a.m))); }

    ENOKI_INLINE auto lt_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f32_u32(vcltq_f32(m, a.m))); }
    ENOKI_INLINE auto gt_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f32_u32(vcgtq_f32(m, a.m))); }
    ENOKI_INLINE auto le_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f32_u32(vcleq_f32(m, a.m))); }
    ENOKI_INLINE auto ge_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f32_u32(vcgeq_f32(m, a.m))); }
    ENOKI_INLINE auto eq_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f32_u32(vceqq_f32(m, a.m))); }
    ENOKI_INLINE auto neq_(Arg a) const { return mask_t<Derived>(vreinterpretq_f32_u32(vmvnq_u32(vceqq_f32(m, a.m)))); }

    ENOKI_INLINE Derived abs_()      const { return vabsq_f32(m); }
    ENOKI_INLINE Derived neg_()      const { return vnegq_f32(m); }
    ENOKI_INLINE Derived not_()      const { return vreinterpretq_f32_s32(vmvnq_s32(vreinterpretq_s32_f32(m))); }

    ENOKI_INLINE Derived min_(Arg b) const { return vminq_f32(b.m, m); }
    ENOKI_INLINE Derived max_(Arg b) const { return vmaxq_f32(b.m, m); }
    ENOKI_INLINE Derived sqrt_()     const { return vsqrtq_f32(m);     }
    ENOKI_INLINE Derived round_()    const { return vrndnq_f32(m);     }
    ENOKI_INLINE Derived floor_()    const { return vrndmq_f32(m);     }
    ENOKI_INLINE Derived ceil_()     const { return vrndpq_f32(m);     }

    ENOKI_INLINE Derived rcp_() const {
        if (Approx) {
            float32x4_t r = vrecpeq_f32(m);
            r = vmulq_f32(r, vrecpsq_f32(r, m));
            r = vmulq_f32(r, vrecpsq_f32(r, m));
            return r;
        } else {
            return Base::rcp_();
        }
    }

    ENOKI_INLINE Derived rsqrt_() const {
        if (Approx) {
            float32x4_t r0 = vrsqrteq_f32(m), r = r0;
            float32x4_t tmp = vmulq_f32(r, m);
            uint32x4_t is_ok = vcgeq_f32(tmp, tmp);
            r = vmulq_f32(r, vrsqrtsq_f32(tmp, r));
            r = vmulq_f32(r, vrsqrtsq_f32(vmulq_f32(r, m), r));
            return vbslq_f32(is_ok, r, r0);
        } else {
            return Base::rsqrt_();
        }
    }

    template <typename Mask_>
    static ENOKI_INLINE Derived select_(const Mask_ &m, Arg t, Arg f) {
        return vbslq_f32(vreinterpretq_u32_f32(m.m), t.m, f.m);
    }

    static constexpr uint64_t shuffle_helper_(int i) {
        if (i == 0)
            return 0x03020100;
        else if (i == 1)
            return 0x07060504;
        else if (i == 2)
            return 0x0B0A0908;
        else
            return 0x0F0E0D0C;
    }

    template <int I0, int I1, int I2, int I3>
    ENOKI_INLINE Derived shuffle_() const {
        /// Based on https://stackoverflow.com/a/32537433/1130282
        switch (I3 + I2*10 + I1*100 + I0*1000) {
            case 0123: return m;
            case 0000: return vdupq_lane_f32(vget_low_f32(m), 0);
            case 1111: return vdupq_lane_f32(vget_low_f32(m), 1);
            case 2222: return vdupq_lane_f32(vget_high_f32(m), 0);
            case 3333: return vdupq_lane_f32(vget_high_f32(m), 1);
            case 1032: return vrev64q_f32(m);
            case 0101: { float32x2_t vt = vget_low_f32(m); return vcombine_f32(vt, vt); }
            case 2323: { float32x2_t vt = vget_high_f32(m); return vcombine_f32(vt, vt); }
            case 1010: { float32x2_t vt = vrev64_f32(vget_low_f32(m)); return vcombine_f32(vt, vt); }
            case 3232: { float32x2_t vt = vrev64_f32(vget_high_f32(m)); return vcombine_f32(vt, vt); }
            case 0132: return vcombine_f32(vget_low_f32(m), vrev64_f32(vget_high_f32(m)));
            case 1023: return vcombine_f32(vrev64_f32(vget_low_f32(m)), vget_high_f32(m));
            case 2310: return vcombine_f32(vget_high_f32(m), vrev64_f32(vget_low_f32(m)));
            case 3201: return vcombine_f32(vrev64_f32(vget_high_f32(m)), vget_low_f32(m));
            case 3210: return vcombine_f32(vrev64_f32(vget_high_f32(m)), vrev64_f32(vget_low_f32(m)));
            case 0022: return vtrn1q_f32(m, m);
            case 1133: return vtrn2q_f32(m, m);
            case 0011: return vzip1q_f32(m, m);
            case 2233: return vzip2q_f32(m, m);
            case 0202: return vuzp1q_f32(m, m);
            case 1313: return vuzp2q_f32(m, m);
            case 1230: return vextq_f32(m, m, 1);
            case 2301: return vextq_f32(m, m, 2);
            case 3012: return vextq_f32(m, m, 3);

            default: {
                constexpr uint64_t prec0 = shuffle_helper_(I0) | (shuffle_helper_(I1) << 32);
                constexpr uint64_t prec1 = shuffle_helper_(I2) | (shuffle_helper_(I3) << 32);

                uint8x8x2_t tbl;
                tbl.val[0] = vreinterpret_u8_f32(vget_low_f32(m));
                tbl.val[1] = vreinterpret_u8_f32(vget_high_f32(m));

                uint8x8_t idx1 = vreinterpret_u8_u32(vcreate_u32(prec0));
                uint8x8_t idx2 = vreinterpret_u8_u32(vcreate_u32(prec1));

                float32x2_t l = vreinterpret_f32_u8(vtbl2_u8(tbl, idx1));
                float32x2_t h = vreinterpret_f32_u8(vtbl2_u8(tbl, idx2));

                return vcombine_f32(l, h);
            }
        }
    }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Horizontal operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Value hmax_() const { return vmaxvq_f32(m); }
    ENOKI_INLINE Value hmin_() const { return vminvq_f32(m); }
    ENOKI_INLINE Value hsum_() const { return vaddvq_f32(m); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Initialization, loading/writing data
    // -----------------------------------------------------------------------

    using Base::load_;
    using Base::store_;
    using Base::load_unaligned_;
    using Base::store_unaligned_;

    ENOKI_INLINE void store_(void *ptr) const {
        vst1q_f32((Value *) ENOKI_ASSUME_ALIGNED_S(ptr, 16), m);
    }

    ENOKI_INLINE void store_unaligned_(void *ptr) const {
        vst1q_f32((Value *) ptr, m);
    }

    static ENOKI_INLINE Derived load_(const void *ptr) {
        return vld1q_f32((const Value *) ENOKI_ASSUME_ALIGNED_S(ptr, 16));
    }

    static ENOKI_INLINE Derived load_unaligned_(const void *ptr) {
        return vld1q_f32((const Value *) ptr);
    }

    //! @}
    // -----------------------------------------------------------------------
};

/// Partial overload of StaticArrayImpl using ARM NEON intrinsics (double precision)
template <bool Approx, typename Derived> struct ENOKI_MAY_ALIAS alignas(16)
    StaticArrayImpl<double, 2, Approx, RoundingMode::Default, Derived>
    : StaticArrayBase<double, 2, Approx, RoundingMode::Default, Derived> {
    ENOKI_NATIVE_ARRAY_CLASSIC(double, 2, Approx, float64x2_t)

    // -----------------------------------------------------------------------
    //! @{ \name Value constructors
    // -----------------------------------------------------------------------

    ENOKI_INLINE StaticArrayImpl(const Value &value) : m(vdupq_n_f64(value)) { }
    ENOKI_INLINE StaticArrayImpl(Value v0, Value v1) : m{v0, v1} { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Type converting constructors
    // -----------------------------------------------------------------------

    ENOKI_CONVERT(double) : m(a.derived().m) { }
    ENOKI_CONVERT(int64_t) : m(vcvtq_f64_s64(vreinterpretq_s64_u64(a.derived().m))) { }
    ENOKI_CONVERT(uint64_t) : m(vcvtq_f64_u64(a.derived().m)) { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Reinterpreting constructors, mask converters
    // -----------------------------------------------------------------------

    ENOKI_REINTERPRET(double) : m(a.derived().m) { }
    ENOKI_REINTERPRET(int64_t) : m(vreinterpretq_f64_u64(a.derived().m)) { }
    ENOKI_REINTERPRET(uint64_t) : m(vreinterpretq_f64_u64(a.derived().m)) { }
    ENOKI_REINTERPRET(bool) {
        m = vreinterpretq_f64_u64(uint64x2_t {
            reinterpret_array<uint64_t>(a.derived().coeff(0)),
            reinterpret_array<uint64_t>(a.derived().coeff(1))
        });
    }
    ENOKI_REINTERPRET(float) {
        auto v0 = memcpy_cast<uint32_t>(a.derived().coeff(0)),
             v1 = memcpy_cast<uint32_t>(a.derived().coeff(1));
        m = vreinterpretq_f64_u32(uint32x4_t { v0, v0, v1, v1 });
    }

    ENOKI_REINTERPRET(int32_t) {
        auto v0 = memcpy_cast<uint32_t>(a.derived().coeff(0)),
             v1 = memcpy_cast<uint32_t>(a.derived().coeff(1));
        m = vreinterpretq_f64_u32(uint32x4_t { v0, v0, v1, v1 });
    }

    ENOKI_REINTERPRET(uint32_t) {
        auto v0 = memcpy_cast<uint32_t>(a.derived().coeff(0)),
             v1 = memcpy_cast<uint32_t>(a.derived().coeff(1));
        m = vreinterpretq_f64_u32(uint32x4_t { v0, v0, v1, v1 });
    }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Converting from/to half size vectors
    // -----------------------------------------------------------------------

    StaticArrayImpl(const Array1 &a1, const Array2 &a2)
        : m(_mm_setr_ps(a1.coeff(0), a2.coeff(0))) { }

    ENOKI_INLINE Array1 low_()  const { return Array1(coeff(0)); }
    ENOKI_INLINE Array2 high_() const { return Array2(coeff(1)); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Vertical operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Derived add_(Arg a) const { return vaddq_f64(m, a.m); }
    ENOKI_INLINE Derived sub_(Arg a) const { return vsubq_f64(m, a.m); }
    ENOKI_INLINE Derived mul_(Arg a) const { return vmulq_f64(m, a.m); }
    ENOKI_INLINE Derived div_(Arg a) const { return vdivq_f64(m, a.m); }

    ENOKI_INLINE Derived fmadd_(Arg b, Arg c) const { return vfmaq_f64(c.m, m, b.m); }
    ENOKI_INLINE Derived fnmadd_(Arg b, Arg c) const { return vfmsq_f64(c.m, m, b.m); }
    ENOKI_INLINE Derived fmsub_(Arg b, Arg c) const { return vfmaq_f64(vnegq_f64(c.m), m, b.m); }
    ENOKI_INLINE Derived fnmsub_(Arg b, Arg c) const { return vfmsq_f64(vnegq_f64(c.m), m, b.m); }

    ENOKI_INLINE Derived or_ (Arg a) const { return vreinterpretq_f64_s64(vorrq_s64(vreinterpretq_s64_f64(m), vreinterpretq_s64_f64(a.m))); }
    ENOKI_INLINE Derived and_(Arg a) const { return vreinterpretq_f64_s64(vandq_s64(vreinterpretq_s64_f64(m), vreinterpretq_s64_f64(a.m))); }
    ENOKI_INLINE Derived xor_(Arg a) const { return vreinterpretq_f64_s64(veorq_s64(vreinterpretq_s64_f64(m), vreinterpretq_s64_f64(a.m))); }

    ENOKI_INLINE auto lt_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f64_u64(vcltq_f64(m, a.m))); }
    ENOKI_INLINE auto gt_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f64_u64(vcgtq_f64(m, a.m))); }
    ENOKI_INLINE auto le_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f64_u64(vcleq_f64(m, a.m))); }
    ENOKI_INLINE auto ge_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f64_u64(vcgeq_f64(m, a.m))); }
    ENOKI_INLINE auto eq_ (Arg a) const { return mask_t<Derived>(vreinterpretq_f64_u64(vceqq_f64(m, a.m))); }
    ENOKI_INLINE auto neq_(Arg a) const { return mask_t<Derived>(vreinterpretq_f64_u64(vmvnq_u64(vceqq_f64(m, a.m)))); }

    ENOKI_INLINE Derived abs_()      const { return vabsq_f64(m); }
    ENOKI_INLINE Derived neg_()      const { return vnegq_f64(m); }
    ENOKI_INLINE Derived not_()      const { return vreinterpretq_f64_s64(vmvnq_s64(vreinterpretq_s64_f64(m))); }

    ENOKI_INLINE Derived min_(Arg b) const { return vminq_f64(b.m, m); }
    ENOKI_INLINE Derived max_(Arg b) const { return vmaxq_f64(b.m, m); }
    ENOKI_INLINE Derived sqrt_()     const { return vsqrtq_f64(m);     }
    ENOKI_INLINE Derived round_()    const { return vrndnq_f64(m);     }
    ENOKI_INLINE Derived floor_()    const { return vrndmq_f64(m);     }
    ENOKI_INLINE Derived ceil_()     const { return vrndpq_f64(m);     }

    ENOKI_INLINE Derived rcp_() const {
        if (Approx) {
            float64x2_t r = vrecpeq_f64(m);
            r = vmulq_f64(r, vrecpsq_f64(r, m));
            r = vmulq_f64(r, vrecpsq_f64(r, m));
            r = vmulq_f64(r, vrecpsq_f64(r, m));
            return r;
        } else {
            return Base::rcp_();
        }
    }

    ENOKI_INLINE Derived rsqrt_() const {
        if (Approx) {
            float64x2_t r0 = vrsqrteq_f64(m), r = r0;
            float64x2_t tmp = vmulq_f64(r, m);
            uint64x2_t is_ok = vcgeq_f64(tmp, tmp);
            r = vmulq_f64(r, vrsqrtsq_f64(tmp, r));
            r = vmulq_f64(r, vrsqrtsq_f64(vmulq_f64(r, m), r));
            r = vmulq_f64(r, vrsqrtsq_f64(vmulq_f64(r, m), r));
            return vbslq_f64(is_ok, r, r0);
        } else {
            return Base::rcp_();
        }
    }

    template <typename Mask_>
    static ENOKI_INLINE Derived select_(const Mask_ &m, Arg t, Arg f) {
        return vbslq_f64(vreinterpretq_u64_f64(m.m), t.m, f.m);
    }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Horizontal operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Value hmax_() const { return vmaxvq_f64(m); }
    ENOKI_INLINE Value hmin_() const { return vminvq_f64(m); }
    ENOKI_INLINE Value hsum_() const { return vaddvq_f64(m); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Initialization, loading/writing data
    // -----------------------------------------------------------------------

    using Base::load_;
    using Base::store_;
    using Base::load_unaligned_;
    using Base::store_unaligned_;

    ENOKI_INLINE void store_(void *ptr) const {
        vst1q_f64((Value *) ENOKI_ASSUME_ALIGNED_S(ptr, 16), m);
    }

    ENOKI_INLINE void store_unaligned_(void *ptr) const {
        vst1q_f64((Value *) ptr, m);
    }

    static ENOKI_INLINE Derived load_(const void *ptr) {
        return vld1q_f64((const Value *) ENOKI_ASSUME_ALIGNED_S(ptr, 16));
    }

    static ENOKI_INLINE Derived load_unaligned_(const void *ptr) {
        return vld1q_f64((const Value *) ptr);
    }

    //! @}
    // -----------------------------------------------------------------------
};

/// Partial overload of StaticArrayImpl using ARM NEON intrinsics (32-bit integers)
template <typename Value_, typename Derived>
struct ENOKI_MAY_ALIAS alignas(16) StaticArrayImpl<Value_, 4, false, RoundingMode::Default,
                                                   Derived, detail::is_int32_t<Value_>>
    : StaticArrayBase<Value_, 4, false, RoundingMode::Default, Derived> {
    ENOKI_NATIVE_ARRAY_CLASSIC(Value_, 4, false, uint32x4_t)

    // -----------------------------------------------------------------------
    //! @{ \name Value constructors
    // -----------------------------------------------------------------------

    ENOKI_INLINE StaticArrayImpl(const Value &value) : m(vdupq_n_u32((uint32_t) value)) { }
    ENOKI_INLINE StaticArrayImpl(Value v0, Value v1, Value v2, Value v3)
        : m{(uint32_t) v0, (uint32_t) v1, (uint32_t) v2, (uint32_t) v3} { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Type converting constructors
    // -----------------------------------------------------------------------

    ENOKI_CONVERT(int32_t) : m(a.derived().m) { }
    ENOKI_CONVERT(uint32_t) : m(a.derived().m) { }
    ENOKI_CONVERT(float) : m(std::is_signed<Value>::value ?
          vreinterpretq_u32_s32(vcvtq_s32_f32(a.derived().m))
        : vcvtq_u32_f32(a.derived().m)) { }
    ENOKI_CONVERT(int64_t) : m(vmovn_high_u64(vmovn_u64(low(a).m), high(a).m)) { }
    ENOKI_CONVERT(uint64_t) : m(vmovn_high_u64(vmovn_u64(low(a).m), high(a).m)) { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Reinterpreting constructors, mask converters
    // -----------------------------------------------------------------------

    ENOKI_REINTERPRET(int32_t) : m(a.derived().m) { }
    ENOKI_REINTERPRET(uint32_t) : m(a.derived().m) { }
    ENOKI_REINTERPRET(int64_t) : m(vcombine_u32(vmovn_u64(low(a).m), vmovn_u64(high(a).m))) { }
    ENOKI_REINTERPRET(uint64_t) : m(vcombine_u32(vmovn_u64(low(a).m), vmovn_u64(high(a).m))) { }
    ENOKI_REINTERPRET(double) : m(vcombine_u32(
        vmovn_u64(vreinterpretq_u64_f64(low(a).m)),
        vmovn_u64(vreinterpretq_u64_f64(high(a).m)))) { }
    ENOKI_REINTERPRET(float) : m(vreinterpretq_u32_f32(a.derived().m)) { }
    ENOKI_REINTERPRET(bool) {
        m = uint32x4_t {
            reinterpret_array<uint32_t>(a.derived().coeff(0)),
            reinterpret_array<uint32_t>(a.derived().coeff(1)),
            reinterpret_array<uint32_t>(a.derived().coeff(2)),
            reinterpret_array<uint32_t>(a.derived().coeff(3))
        };
    }

    //! @}
    // -----------------------------------------------------------------------


    // -----------------------------------------------------------------------
    //! @{ \name Converting from/to half size vectors
    // -----------------------------------------------------------------------

    StaticArrayImpl(const Array1 &a1, const Array2 &a2)
        : m(_mm_setr_ps(a1.coeff(0), a1.coeff(1), a2.coeff(0), a2.coeff(1))) { }

    ENOKI_INLINE Array1 low_()  const { return Array1(coeff(0), coeff(1)); }
    ENOKI_INLINE Array2 high_() const { return Array2(coeff(2), coeff(3)); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Vertical operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Derived add_(Arg a) const { return vaddq_u32(m, a.m); }
    ENOKI_INLINE Derived sub_(Arg a) const { return vsubq_u32(m, a.m); }
    ENOKI_INLINE Derived mul_(Arg a) const { return vmulq_u32(m, a.m); }

    ENOKI_INLINE Derived or_ (Arg a) const { return vorrq_u32(m, a.m); }
    ENOKI_INLINE Derived and_(Arg a) const { return vandq_u32(m, a.m); }
    ENOKI_INLINE Derived xor_(Arg a) const { return veorq_u32(m, a.m); }

    ENOKI_INLINE auto lt_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcltq_s32(vreinterpretq_s32_u32(m), vreinterpretq_s32_u32(a.m)));
        else
            return mask_t<Derived>(vcltq_u32(m, a.m));
    }

    ENOKI_INLINE auto gt_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcgtq_s32(vreinterpretq_s32_u32(m), vreinterpretq_s32_u32(a.m)));
        else
            return mask_t<Derived>(vcgtq_u32(m, a.m));
    }

    ENOKI_INLINE auto le_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcleq_s32(vreinterpretq_s32_u32(m), vreinterpretq_s32_u32(a.m)));
        else
            return mask_t<Derived>(vcleq_u32(m, a.m));
    }

    ENOKI_INLINE auto ge_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcgeq_s32(vreinterpretq_s32_u32(m), vreinterpretq_s32_u32(a.m)));
        else
            return mask_t<Derived>(vcgeq_u32(m, a.m));
    }

    ENOKI_INLINE auto eq_ (Arg a) const { return mask_t<Derived>(vceqq_u32(m, a.m)); }
    ENOKI_INLINE auto neq_(Arg a) const { return mask_t<Derived>(vmvnq_u32(vceqq_u32(m, a.m))); }

    ENOKI_INLINE Derived abs_() const {
        if (!std::is_signed<Value>())
            return m;
        return vreinterpretq_u32_s32(vabsq_s32(vreinterpretq_s32_u32(m)));
    }

    ENOKI_INLINE Derived neg_() const {
        static_assert(std::is_signed<Value>::value, "Expected a signed value!");
        return vreinterpretq_u32_s32(vnegq_s32(vreinterpretq_s32_u32(m)));
    }

    ENOKI_INLINE Derived not_()      const { return vmvnq_u32(m); }

    ENOKI_INLINE Derived max_(Arg b) const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u32_s32(vmaxq_s32(vreinterpretq_s32_u32(b.m), vreinterpretq_s32_u32(m)));
        else
            return vmaxq_u32(b.m, m);
    }

    ENOKI_INLINE Derived min_(Arg b) const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u32_s32(vminq_s32(vreinterpretq_s32_u32(b.m), vreinterpretq_s32_u32(m)));
        else
            return vminq_u32(b.m, m);
    }

    template <typename Mask_>
    static ENOKI_INLINE Derived select_(const Mask_ &m, Arg t, Arg f) {
        return vbslq_u32(m.m, t.m, f.m);
    }

    template <size_t Imm> ENOKI_INLINE Derived sri_() const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u32_s32(
                vshrq_n_s32(vreinterpretq_s32_u32(m), (int) Imm));
        else
            return vshrq_n_u32(m, (int) Imm);
    }

    template <size_t Imm> ENOKI_INLINE Derived sli_() const {
        return vshlq_n_u32(m, (int) Imm);
    }

    ENOKI_INLINE Derived sr_(size_t k) const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u32_s32(
                vshlq_s32(vreinterpretq_s32_u32(m), vdupq_n_s32(-(int) k)));
        else
            return vshlq_u32(m, vdupq_n_s32(-(int) k));
    }

    ENOKI_INLINE Derived sl_(size_t k) const {
        return vshlq_u32(m, vdupq_n_s32((int) k));
    }

    ENOKI_INLINE Derived srv_(Arg a) const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u32_s32(
                vshlq_s32(vreinterpretq_s32_u32(m),
                          vnegq_s32(vreinterpretq_s32_u32(a.m))));
        else
            return vshlq_u32(m, vnegq_s32(vreinterpretq_s32_u32(a.m)));
    }

    ENOKI_INLINE Derived slv_(Arg a) const {
        return vshlq_u32(m, vreinterpretq_s32_u32(a.m));
    }

    ENOKI_INLINE Derived mulhi_(Arg a) const {
        if (std::is_signed<Value>::value) {
            int64x2_t l = vmull_s32(vreinterpret_s32_u32(vget_low_u32(m)),
                                    vreinterpret_s32_u32(vget_low_u32(a.m)));

            int64x2_t h = vmull_high_s32(vreinterpretq_s32_u32(m),
                                         vreinterpretq_s32_u32(a.m));

            return vreinterpretq_u32_s32(vuzp2q_s32(vreinterpretq_s32_s64(l),
                                                    vreinterpretq_s32_s64(h)));
        } else {
            uint64x2_t l = vmull_u32(vget_low_u32(m),
                                     vget_low_u32(a.m));

            uint64x2_t h = vmull_high_u32(m, a.m);

            return vuzp2q_u32(vreinterpretq_u32_u64(l),
                              vreinterpretq_u32_u64(h));
        }
    }

    ENOKI_INLINE Derived lzcnt_() const { return vclzq_u32(m); }
    ENOKI_INLINE Derived tzcnt_() const { return Value(32) - lzcnt(~derived() & (derived() - Value(1))); }
    ENOKI_INLINE Derived popcnt_() const { return vpaddlq_u16(vpaddlq_u8(vcntq_u8(vreinterpretq_u8_u32(m)))); }

    static constexpr uint64_t shuffle_helper_(int i) {
        if (i == 0)
            return 0x03020100;
        else if (i == 1)
            return 0x07060504;
        else if (i == 2)
            return 0x0B0A0908;
        else
            return 0x0F0E0D0C;
    }

    template <int I0, int I1, int I2, int I3>
    ENOKI_INLINE Derived shuffle_() const {
        /// Based on https://stackoverflow.com/a/32537433/1130282
        switch (I3 + I2*10 + I1*100 + I0*1000) {
            case 0123: return m;
            case 0000: return vdupq_lane_u32(vget_low_u32(m), 0);
            case 1111: return vdupq_lane_u32(vget_low_u32(m), 1);
            case 2222: return vdupq_lane_u32(vget_high_u32(m), 0);
            case 3333: return vdupq_lane_u32(vget_high_u32(m), 1);
            case 1032: return vrev64q_u32(m);
            case 0101: { uint32x2_t vt = vget_low_u32(m); return vcombine_u32(vt, vt); }
            case 2323: { uint32x2_t vt = vget_high_u32(m); return vcombine_u32(vt, vt); }
            case 1010: { uint32x2_t vt = vrev64_u32(vget_low_u32(m)); return vcombine_u32(vt, vt); }
            case 3232: { uint32x2_t vt = vrev64_u32(vget_high_u32(m)); return vcombine_u32(vt, vt); }
            case 0132: return vcombine_u32(vget_low_u32(m), vrev64_u32(vget_high_u32(m)));
            case 1023: return vcombine_u32(vrev64_u32(vget_low_u32(m)), vget_high_u32(m));
            case 2310: return vcombine_u32(vget_high_u32(m), vrev64_u32(vget_low_u32(m)));
            case 3201: return vcombine_u32(vrev64_u32(vget_high_u32(m)), vget_low_u32(m));
            case 3210: return vcombine_u32(vrev64_u32(vget_high_u32(m)), vrev64_u32(vget_low_u32(m)));
            case 0022: return vtrn1q_u32(m, m);
            case 1133: return vtrn2q_u32(m, m);
            case 0011: return vzip1q_u32(m, m);
            case 2233: return vzip2q_u32(m, m);
            case 0202: return vuzp1q_u32(m, m);
            case 1313: return vuzp2q_u32(m, m);
            case 1230: return vextq_u32(m, m, 1);
            case 2301: return vextq_u32(m, m, 2);
            case 3012: return vextq_u32(m, m, 3);

            default: {
                constexpr uint64_t prec0 = shuffle_helper_(I0) | (shuffle_helper_(I1) << 32);
                constexpr uint64_t prec1 = shuffle_helper_(I2) | (shuffle_helper_(I3) << 32);

                uint8x8x2_t tbl;
                tbl.val[0] = vreinterpret_u8_u32(vget_low_u32(m));
                tbl.val[1] = vreinterpret_u8_u32(vget_high_u32(m));

                uint8x8_t idx1 = vreinterpret_u8_u32(vcreate_u32(prec0));
                uint8x8_t idx2 = vreinterpret_u8_u32(vcreate_u32(prec1));

                uint32x2_t l = vreinterpret_u32_u8(vtbl2_u8(tbl, idx1));
                uint32x2_t h = vreinterpret_u32_u8(vtbl2_u8(tbl, idx2));

                return vcombine_u32(l, h);
            }
        }
    }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Horizontal operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Value hmax_() const {
        if (std::is_signed<Value>::value)
            return Value(vmaxvq_s32(vreinterpretq_s32_u32(m)));
        else
            return Value(vmaxvq_u32(m));
    }

    ENOKI_INLINE Value hmin_() const {
        if (std::is_signed<Value>::value)
            return Value(vminvq_s32(vreinterpretq_s32_u32(m)));
        else
            return Value(vminvq_u32(m));
    }

    ENOKI_INLINE Value hsum_() const { return Value(vaddvq_u32(m)); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Initialization, loading/writing data
    // -----------------------------------------------------------------------

    using Base::load_;
    using Base::store_;
    using Base::load_unaligned_;
    using Base::store_unaligned_;

    ENOKI_INLINE void store_(void *ptr) const {
        vst1q_u32((uint32_t *) ENOKI_ASSUME_ALIGNED_S(ptr, 16), m);
    }

    ENOKI_INLINE void store_unaligned_(void *ptr) const {
        vst1q_u32((uint32_t *) ptr, m);
    }

    static ENOKI_INLINE Derived load_(const void *ptr) {
        return vld1q_u32((const uint32_t *) ENOKI_ASSUME_ALIGNED_S(ptr, 16));
    }

    static ENOKI_INLINE Derived load_unaligned_(const void *ptr) {
        return vld1q_u32((const uint32_t *) ptr);
    }

    //! @}
    // -----------------------------------------------------------------------
};

/// Partial overload of StaticArrayImpl using ARM NEON intrinsics (64-bit integers)
template <typename Value_, typename Derived>
struct ENOKI_MAY_ALIAS alignas(16) StaticArrayImpl<Value_, 2, false, RoundingMode::Default,
                                                   Derived, detail::is_int64_t<Value_>>
    : StaticArrayBase<Value_, 2, false, RoundingMode::Default, Derived> {
    ENOKI_NATIVE_ARRAY_CLASSIC(Value_, 2, false, uint64x2_t)

    // -----------------------------------------------------------------------
    //! @{ \name Value constructors
    // -----------------------------------------------------------------------

    ENOKI_INLINE StaticArrayImpl(const Value &value) : m(vdupq_n_u64((uint64_t) value)) { }
    ENOKI_INLINE StaticArrayImpl(Value v0, Value v1) : m{(uint64_t) v0, (uint64_t) v1} { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Type converting constructors
    // -----------------------------------------------------------------------

    ENOKI_CONVERT(int64_t) : m(a.derived().m) { }
    ENOKI_CONVERT(uint64_t) : m(a.derived().m) { }
    ENOKI_CONVERT(double) : m(std::is_signed<Value>::value ?
          vreinterpretq_u64_s64(vcvtq_s64_f64(a.derived().m))
        : vcvtq_u64_f64(a.derived().m)) { }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Reinterpreting constructors, mask converters
    // -----------------------------------------------------------------------

    ENOKI_REINTERPRET(int64_t) : m(a.derived().m) { }
    ENOKI_REINTERPRET(uint64_t) : m(a.derived().m) { }
    ENOKI_REINTERPRET(double) : m(vreinterpretq_u64_f64(a.derived().m)) { }
    ENOKI_REINTERPRET(bool) {
        m = uint64x2_t {
            reinterpret_array<uint64_t>(a.derived().coeff(0)),
            reinterpret_array<uint64_t>(a.derived().coeff(1))
        };
    }
    ENOKI_REINTERPRET(float) {
        auto v0 = memcpy_cast<uint32_t>(a.derived().coeff(0)),
             v1 = memcpy_cast<uint32_t>(a.derived().coeff(1));
        m = vreinterpretq_u64_u32(uint32x4_t { v0, v0, v1, v1 });
    }

    ENOKI_REINTERPRET(int32_t) {
        auto v0 = memcpy_cast<uint32_t>(a.derived().coeff(0)),
             v1 = memcpy_cast<uint32_t>(a.derived().coeff(1));
        m = vreinterpretq_u64_u32(uint32x4_t { v0, v0, v1, v1 });
    }

    ENOKI_REINTERPRET(uint32_t) {
        auto v0 = memcpy_cast<uint32_t>(a.derived().coeff(0)),
             v1 = memcpy_cast<uint32_t>(a.derived().coeff(1));
        m = vreinterpretq_u64_u32(uint32x4_t { v0, v0, v1, v1 });
    }

    //! @}
    // -----------------------------------------------------------------------


    // -----------------------------------------------------------------------
    //! @{ \name Converting from/to half size vectors
    // -----------------------------------------------------------------------

    StaticArrayImpl(const Array1 &a1, const Array2 &a2)
        : m(_mm_setr_ps(a1.coeff(0), a2.coeff(0))) { }

    ENOKI_INLINE Array1 low_()  const { return Array1(coeff(0)); }
    ENOKI_INLINE Array2 high_() const { return Array2(coeff(1)); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Vertical operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Derived add_(Arg a) const { return vaddq_u64(m, a.m); }
    ENOKI_INLINE Derived sub_(Arg a) const { return vsubq_u64(m, a.m); }
    ENOKI_INLINE Derived mul_(Arg a_) const {
#if 1
        // Native ARM instructions + cross-domain penalities still
        // seem to be faster than the NEON approach below
        return Derived(
            coeff(0) * a_.coeff(0),
            coeff(1) * a_.coeff(1)
        );
#else
        // inp: [ah0, al0, ah1, al1], [bh0, bl0, bh1, bl1]
        uint32x4_t a = vreinterpretq_u32_u64(m),
                   b = vreinterpretq_u32_u64(a_.m);

        // uzp: [al0, al1, bl0, bl1], [bh0, bh1, ah0, ah1]
        uint32x4_t l = vuzp1q_u32(a, b);
        uint32x4_t h = vuzp2q_u32(b, a);

        uint64x2_t accum = vmull_u32(vget_low_u32(l), vget_low_u32(h));
        accum = vmlal_high_u32(accum, h, l);
        accum = vshlq_n_u64(accum, 32);
        accum = vmlal_u32(accum, vget_low_u32(l), vget_high_u32(l));

        return accum;
#endif
    }

    ENOKI_INLINE Derived or_ (Arg a) const { return vorrq_u64(m, a.m); }
    ENOKI_INLINE Derived and_(Arg a) const { return vandq_u64(m, a.m); }
    ENOKI_INLINE Derived xor_(Arg a) const { return veorq_u64(m, a.m); }

    ENOKI_INLINE auto lt_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcltq_s64(vreinterpretq_s64_u64(m), vreinterpretq_s64_u64(a.m)));
        else
            return mask_t<Derived>(vcltq_u64(m, a.m));
    }

    ENOKI_INLINE auto gt_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcgtq_s64(vreinterpretq_s64_u64(m), vreinterpretq_s64_u64(a.m)));
        else
            return mask_t<Derived>(vcgtq_u64(m, a.m));
    }

    ENOKI_INLINE auto le_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcleq_s64(vreinterpretq_s64_u64(m), vreinterpretq_s64_u64(a.m)));
        else
            return mask_t<Derived>(vcleq_u64(m, a.m));
    }

    ENOKI_INLINE auto ge_(Arg a) const {
        if (std::is_signed<Value>::value)
            return mask_t<Derived>(vcgeq_s64(vreinterpretq_s64_u64(m), vreinterpretq_s64_u64(a.m)));
        else
            return mask_t<Derived>(vcgeq_u64(m, a.m));
    }

    ENOKI_INLINE auto eq_ (Arg a) const { return mask_t<Derived>(vceqq_u64(m, a.m)); }
    ENOKI_INLINE auto neq_(Arg a) const { return mask_t<Derived>(vmvnq_u64(vceqq_u64(m, a.m))); }

    ENOKI_INLINE Derived abs_() const {
        if (!std::is_signed<Value>())
            return m;
        return vreinterpretq_u64_s64(vabsq_s64(vreinterpretq_s64_u64(m)));
    }

    ENOKI_INLINE Derived neg_() const {
        static_assert(std::is_signed<Value>::value, "Expected a signed value!");
        return vreinterpretq_u64_s64(vnegq_s64(vreinterpretq_s64_u64(m)));
    }

    ENOKI_INLINE Derived not_()      const { return vmvnq_u64(m); }

    ENOKI_INLINE Derived min_(Arg b) const { return Derived(min(coeff(0), b.coeff(0)), min(coeff(1), b.coeff(1))); }
    ENOKI_INLINE Derived max_(Arg b) const { return Derived(max(coeff(0), b.coeff(0)), max(coeff(1), b.coeff(1))); }

    template <typename Mask_>
    static ENOKI_INLINE Derived select_(const Mask_ &m, Arg t, Arg f) {
        return vbslq_u64(m.m, t.m, f.m);
    }

    template <size_t Imm> ENOKI_INLINE Derived sri_() const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u64_s64(
                vshrq_n_s64(vreinterpretq_s64_u64(m), (int) Imm));
        else
            return vshrq_n_u64(m, (int) Imm);
    }

    template <size_t Imm> ENOKI_INLINE Derived sli_() const {
        return vshlq_n_u64(m, (int) Imm);
    }

    ENOKI_INLINE Derived sr_(size_t k) const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u64_s64(
                vshlq_s64(vreinterpretq_s64_u64(m), vdupq_n_s64(-(int) k)));
        else
            return vshlq_u64(m, vdupq_n_s64(-(int) k));
    }

    ENOKI_INLINE Derived sl_(size_t k) const {
        return vshlq_u64(m, vdupq_n_s64((int) k));
    }

    ENOKI_INLINE Derived srv_(Arg a) const {
        if (std::is_signed<Value>::value)
            return vreinterpretq_u64_s64(
                vshlq_s64(vreinterpretq_s64_u64(m),
                          vnegq_s64(vreinterpretq_s64_u64(a.m))));
        else
            return vshlq_u64(m, vnegq_s64(vreinterpretq_s64_u64(a.m)));
    }

    ENOKI_INLINE Derived slv_(Arg a) const {
        return vshlq_u64(m, vreinterpretq_s64_u64(a.m));
    }

    ENOKI_INLINE Derived popcnt_() const { return vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vcntq_u8(vreinterpretq_u8_u64(m))))); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Horizontal operations
    // -----------------------------------------------------------------------

    ENOKI_INLINE Value hsum_() const { return Value(vaddvq_u64(m)); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Initialization, loading/writing data
    // -----------------------------------------------------------------------

    using Base::load_;
    using Base::store_;
    using Base::load_unaligned_;
    using Base::store_unaligned_;

    ENOKI_INLINE void store_(void *ptr) const {
        vst1q_u64((uint64_t *) ENOKI_ASSUME_ALIGNED_S(ptr, 16), m);
    }

    ENOKI_INLINE void store_unaligned_(void *ptr) const {
        vst1q_u64((uint64_t *) ptr, m);
    }

    static ENOKI_INLINE Derived load_(const void *ptr) {
        return vld1q_u64((const uint64_t *) ENOKI_ASSUME_ALIGNED_S(ptr, 16));
    }

    static ENOKI_INLINE Derived load_unaligned_(const void *ptr) {
        return vld1q_u64((const uint64_t *) ptr);
    }

    //! @}
    // -----------------------------------------------------------------------
};

/// Partial overload of StaticArrayImpl for the n=3 case (single precision)
template <bool Approx, typename Derived> struct ENOKI_MAY_ALIAS alignas(16)
    StaticArrayImpl<float, 3, Approx, RoundingMode::Default, Derived>
    : StaticArrayImpl<float, 4, Approx, RoundingMode::Default, Derived> {
    using Base = StaticArrayImpl<float, 4, Approx, RoundingMode::Default, Derived>;
    using Mask = detail::ArrayMask<float, 3, Approx, RoundingMode::Default>;

    using typename Base::Value;
    using Arg = const Base &;
    using Base::Base;
    using Base::m;
    using Base::operator=;
    using Base::coeff;
    using Base::derived;
    static constexpr size_t Size = 3;

    ENOKI_INLINE StaticArrayImpl(Value f0, Value f1, Value f2) : Base(f0, f1, f2, Value(0)) { }
    ENOKI_INLINE StaticArrayImpl() : Base() { }

    ENOKI_REINTERPRET(bool) {
        m = vreinterpretq_f32_u32(uint32x4_t {
            reinterpret_array<uint32_t>(a.derived().coeff(0)),
            reinterpret_array<uint32_t>(a.derived().coeff(1)),
            reinterpret_array<uint32_t>(a.derived().coeff(2)),
            0u
        });
    }

    StaticArrayImpl(const StaticArrayImpl &) = default;
    StaticArrayImpl &operator=(const StaticArrayImpl &) = default;

    template <
        typename Value2, bool Approx2, RoundingMode Mode2, typename Derived2>
    ENOKI_INLINE StaticArrayImpl(
        const StaticArrayBase<Value2, 3, Approx2, Mode2, Derived2> &a) {
        ENOKI_TRACK_SCALAR for (size_t i = 0; i < 3; ++i)
            coeff(i) = Value(a.derived().coeff(i));
    }

    template <bool Approx2, RoundingMode Mode2, typename Derived2>
    ENOKI_INLINE StaticArrayImpl(
        const StaticArrayBase<half, 3, Approx2, Mode2, Derived2> &a) {
        float16x4_t value;
        memcpy(&value, a.data(), sizeof(uint16_t)*3);
        m = vcvt_f32_f16(value);
    }

    template <int I0, int I1, int I2>
    ENOKI_INLINE Derived shuffle_() const {
        /// Based on https://stackoverflow.com/a/32537433/1130282
        switch (I2 + I1*10 + I0*100) {
            case 012: return m;
            case 000: return vdupq_lane_f32(vget_low_f32(m), 0);
            case 111: return vdupq_lane_f32(vget_low_f32(m), 1);
            case 222: return vdupq_lane_f32(vget_high_f32(m), 0);
            case 333: return vdupq_lane_f32(vget_high_f32(m), 1);
            case 103: return vrev64q_f32(m);
            case 010: { float32x2_t vt = vget_low_f32(m); return vcombine_f32(vt, vt); }
            case 232: { float32x2_t vt = vget_high_f32(m); return vcombine_f32(vt, vt); }
            case 101: { float32x2_t vt = vrev64_f32(vget_low_f32(m)); return vcombine_f32(vt, vt); }
            case 323: { float32x2_t vt = vrev64_f32(vget_high_f32(m)); return vcombine_f32(vt, vt); }
            case 013: return vcombine_f32(vget_low_f32(m), vrev64_f32(vget_high_f32(m)));
            case 102: return vcombine_f32(vrev64_f32(vget_low_f32(m)), vget_high_f32(m));
            case 231: return vcombine_f32(vget_high_f32(m), vrev64_f32(vget_low_f32(m)));
            case 320: return vcombine_f32(vrev64_f32(vget_high_f32(m)), vget_low_f32(m));
            case 321: return vcombine_f32(vrev64_f32(vget_high_f32(m)), vrev64_f32(vget_low_f32(m)));
            case 002: return vtrn1q_f32(m, m);
            case 113: return vtrn2q_f32(m, m);
            case 001: return vzip1q_f32(m, m);
            case 223: return vzip2q_f32(m, m);
            case 020: return vuzp1q_f32(m, m);
            case 131: return vuzp2q_f32(m, m);
            case 123: return vextq_f32(m, m, 1);
            case 230: return vextq_f32(m, m, 2);
            case 301: return vextq_f32(m, m, 3);
            default: return Base::template shuffle_<I0, I1, I2, 3>();
        }
    }

    // -----------------------------------------------------------------------
    //! @{ \name Horizontal operations (adapted for the n=3 case)
    // -----------------------------------------------------------------------

    ENOKI_INLINE Value hmax_() const { return max(max(coeff(0), coeff(1)), coeff(2)); }
    ENOKI_INLINE Value hmin_() const { return min(min(coeff(0), coeff(1)), coeff(2)); }
    ENOKI_INLINE Value hsum_() const { return coeff(0) + coeff(1) + coeff(2); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Loading/writing data (adapted for the n=3 case)
    // -----------------------------------------------------------------------

    using Base::load_;
    using Base::store_;
    using Base::load_unaligned_;
    using Base::store_unaligned_;

    ENOKI_INLINE void store_(void *ptr) const {
        memcpy(ptr, &m, sizeof(Value) * 3);
    }

    ENOKI_INLINE void store_unaligned_(void *ptr) const {
        store_(ptr);
    }

    static ENOKI_INLINE Derived load_unaligned_(const void *ptr) {
        Derived result;
        memcpy(&result.m, ptr, sizeof(Value) * 3);
        return result;
    }

    static ENOKI_INLINE Derived load_(const void *ptr) {
        return Base::load_unaligned_(ptr);
    }

    //! @}
    // -----------------------------------------------------------------------
};

/// Partial overload of StaticArrayImpl for the n=3 case (32 bit integers)
template <typename Value_, typename Derived> struct ENOKI_MAY_ALIAS alignas(16)
    StaticArrayImpl<Value_, 3, false, RoundingMode::Default, Derived,detail::is_int32_t<Value_>>
    : StaticArrayImpl<Value_, 4, false, RoundingMode::Default, Derived> {
    using Base = StaticArrayImpl<Value_, 4, false, RoundingMode::Default, Derived>;
    using Mask = detail::ArrayMask<Value_, 3, false, RoundingMode::Default>;

    using typename Base::Value;
    using Arg = const Base &;
    using Base::Base;
    using Base::m;
    using Base::operator=;
    using Base::coeff;
    using Base::derived;
    static constexpr size_t Size = 3;

    ENOKI_INLINE StaticArrayImpl(Value f0, Value f1, Value f2) : Base(f0, f1, f2, Value(0)) { }
    ENOKI_INLINE StaticArrayImpl() : Base() { }

    StaticArrayImpl(const StaticArrayImpl &) = default;
    StaticArrayImpl &operator=(const StaticArrayImpl &) = default;

    template <
        typename Value2, bool Approx2, RoundingMode Mode2, typename Derived2>
    ENOKI_INLINE StaticArrayImpl(
        const StaticArrayBase<Value2, 3, Approx2, Mode2, Derived2> &a) {
        ENOKI_TRACK_SCALAR for (size_t i = 0; i < 3; ++i)
            coeff(i) = Value(a.derived().coeff(i));
    }

    ENOKI_REINTERPRET(bool) {
        m = uint32x4_t {
            reinterpret_array<uint32_t>(a.derived().coeff(0)),
            reinterpret_array<uint32_t>(a.derived().coeff(1)),
            reinterpret_array<uint32_t>(a.derived().coeff(2)),
            0u
        };
    }

    template <int I0, int I1, int I2>
    ENOKI_INLINE Derived shuffle_() const {
        /// Based on https://stackoverflow.com/a/32537433/1130282
        switch (I2 + I1*10 + I0*100) {
            case 012: return m;
            case 000: return vdupq_lane_u32(vget_low_u32(m), 0);
            case 111: return vdupq_lane_u32(vget_low_u32(m), 1);
            case 222: return vdupq_lane_u32(vget_high_u32(m), 0);
            case 333: return vdupq_lane_u32(vget_high_u32(m), 1);
            case 103: return vrev64q_u32(m);
            case 010: { uint32x2_t vt = vget_low_u32(m); return vcombine_u32(vt, vt); }
            case 232: { uint32x2_t vt = vget_high_u32(m); return vcombine_u32(vt, vt); }
            case 101: { uint32x2_t vt = vrev64_u32(vget_low_u32(m)); return vcombine_u32(vt, vt); }
            case 323: { uint32x2_t vt = vrev64_u32(vget_high_u32(m)); return vcombine_u32(vt, vt); }
            case 013: return vcombine_u32(vget_low_u32(m), vrev64_u32(vget_high_u32(m)));
            case 102: return vcombine_u32(vrev64_u32(vget_low_u32(m)), vget_high_u32(m));
            case 231: return vcombine_u32(vget_high_u32(m), vrev64_u32(vget_low_u32(m)));
            case 320: return vcombine_u32(vrev64_u32(vget_high_u32(m)), vget_low_u32(m));
            case 321: return vcombine_u32(vrev64_u32(vget_high_u32(m)), vrev64_u32(vget_low_u32(m)));
            case 002: return vtrn1q_u32(m, m);
            case 113: return vtrn2q_u32(m, m);
            case 001: return vzip1q_u32(m, m);
            case 223: return vzip2q_u32(m, m);
            case 020: return vuzp1q_u32(m, m);
            case 131: return vuzp2q_u32(m, m);
            case 123: return vextq_u32(m, m, 1);
            case 230: return vextq_u32(m, m, 2);
            case 301: return vextq_u32(m, m, 3);
            default: return Base::template shuffle_<I0, I1, I2, 3>();
        }
    }

    // -----------------------------------------------------------------------
    //! @{ \name Horizontal operations (adapted for the n=3 case)
    // -----------------------------------------------------------------------

    ENOKI_INLINE Value hmax_() const { return max(max(coeff(0), coeff(1)), coeff(2)); }
    ENOKI_INLINE Value hmin_() const { return min(min(coeff(0), coeff(1)), coeff(2)); }
    ENOKI_INLINE Value hsum_() const { return coeff(0) + coeff(1) + coeff(2); }

    //! @}
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //! @{ \name Loading/writing data (adapted for the n=3 case)
    // -----------------------------------------------------------------------

    using Base::load_;
    using Base::store_;
    using Base::load_unaligned_;
    using Base::store_unaligned_;

    ENOKI_INLINE void store_(void *ptr) const {
        memcpy(ptr, &m, sizeof(Value) * 3);
    }

    ENOKI_INLINE void store_unaligned_(void *ptr) const {
        store_(ptr);
    }

    static ENOKI_INLINE Derived load_unaligned_(const void *ptr) {
        Derived result;
        memcpy(&result.m, ptr, sizeof(Value) * 3);
        return result;
    }

    static ENOKI_INLINE Derived load_(const void *ptr) {
        return Base::load_unaligned_(ptr);
    }

    //! @}
    // -----------------------------------------------------------------------
};

NAMESPACE_END(enoki)