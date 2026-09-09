// Diagnostic platform-math isolation, not a replacement K1 algorithm or golden.
// Table and polynomial: Arm optimized-routines v23.01 math/logf{,_data}.c.
// Copyright (c) 2017-2023, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
// Explicit FMAs reproduce the installed newlib 4.4.0 M85 disassembly. The
// probe script independently verifies all 36 table doubles against that ELF.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
extern "C" const double k1_probe_logf_data[] = {
  0x1.661ec79f8f3bep+0, -0x1.57bf7808caadep-2,
  0x1.571ed4aaf883dp+0, -0x1.2bef0a7c06ddbp-2,
  0x1.49539f0f010bp+0, -0x1.01eae7f513a67p-2,
  0x1.3c995b0b80385p+0, -0x1.b31d8a68224e9p-3,
  0x1.30d190c8864a5p+0, -0x1.6574f0ac07758p-3,
  0x1.25e227b0b8eap+0, -0x1.1aa2bc79c81p-3,
  0x1.1bb4a4a1a343fp+0, -0x1.a4e76ce8c0e5ep-4,
  0x1.12358f08ae5bap+0, -0x1.1973c5a611cccp-4,
  0x1.0953f419900a7p+0, -0x1.252f438e10c1ep-5,
  0x1p+0, 0x0p+0,
  0x1.e608cfd9a47acp-1, 0x1.aa5aa5df25984p-5,
  0x1.ca4b31f026aap-1, 0x1.c5e53aa362eb4p-4,
  0x1.b2036576afce6p-1, 0x1.526e57720db08p-3,
  0x1.9c2d163a1aa2dp-1, 0x1.bc2860d22477p-3,
  0x1.886e6037841edp-1, 0x1.1058bc8a07ee1p-2,
  0x1.767dcf5534862p-1, 0x1.4043057b6ee09p-2,
  0x1.62e42fefa39efp-1,
  -0x1.00ea348b88334p-2, 0x1.5575b0be00b6ap-2, -0x1.ffffef20a4123p-2
};
extern "C" float logf(float x) {
  std::uint32_t bits; std::memcpy(&bits, &x, 4);
  if (bits == 0x3f800000) return 0;
  if (bits - 0x00800000U >= 0x7f000000U) {
    if ((bits << 1) == 0) return -INFINITY;
    if (bits == 0x7f800000U) return x;
    if ((bits & 0x80000000U) || (bits << 1) >= 0xff000000U) return NAN;
    x *= 0x1p23F; std::memcpy(&bits, &x, 4); bits -= 23U << 23;
  }
  const std::uint32_t tmp = bits - 0x3f330000U;
  const unsigned i = (tmp >> 19) & 15;
  const int k = static_cast<std::int32_t>(tmp) >> 23;
  bits -= tmp & 0xff800000U;
  float z; std::memcpy(&z, &bits, 4);
  const auto* t = k1_probe_logf_data;
  const double r = std::fma(double(z), t[2*i], -1.0);
  const double y0 = std::fma(double(k), t[32], t[2*i+1]);
  const double r2 = r*r;
  double y = std::fma(t[34], r, t[35]);
  y = std::fma(t[33], r2, y);
  return static_cast<float>(std::fma(y, r2, y0+r));
}

// expf/exp2f_data from the same Arm release, with the linked M85 FMA order.
extern "C" const std::uint64_t k1_probe_expf_table[] = {
  0x3ff0000000000000, 0x3fefd9b0d3158574, 0x3fefb5586cf9890f, 0x3fef9301d0125b51,
  0x3fef72b83c7d517b, 0x3fef54873168b9aa, 0x3fef387a6e756238, 0x3fef1e9df51fdee1,
  0x3fef06fe0a31b715, 0x3feef1a7373aa9cb, 0x3feedea64c123422, 0x3feece086061892d,
  0x3feebfdad5362a27, 0x3feeb42b569d4f82, 0x3feeab07dd485429, 0x3feea47eb03a5585,
  0x3feea09e667f3bcd, 0x3fee9f75e8ec5f74, 0x3feea11473eb0187, 0x3feea589994cce13,
  0x3feeace5422aa0db, 0x3feeb737b0cdc5e5, 0x3feec49182a3f090, 0x3feed503b23e255d,
  0x3feee89f995ad3ad, 0x3feeff76f2fb5e47, 0x3fef199bdd85529c, 0x3fef3720dcef9069,
  0x3fef5818dcfba487, 0x3fef7c97337b9b5f, 0x3fefa4afa2a490da, 0x3fefd0765b6e4540
};
extern "C" const double k1_probe_expf_constants[] = {
  0x1.8p52/32, 0x1.c6af84b912394p-5, 0x1.ebfce50fac4f3p-3, 0x1.62e42ff0c52d6p-1,
  0x1.8p52, 0x1.71547652b82fep+0*32,
  0x1.c6af84b912394p-5/32/32/32, 0x1.ebfce50fac4f3p-3/32/32, 0x1.62e42ff0c52d6p-1/32
};
extern "C" float expf(float x) {
  // Only the normal finite domain used by this K1 diagnostic is modelled.
  if (!std::isfinite(x) || std::fabs(x) >= 88.0F) std::abort();
  const auto* c = k1_probe_expf_constants;
  const double kd = std::fma(c[5], double(x), c[4]);
  std::uint64_t ki; std::memcpy(&ki, &kd, 8);
  const double r = std::fma(c[5], double(x), -(kd-c[4]));
  const std::uint64_t bits = k1_probe_expf_table[ki%32] + (ki << 47);
  double s; std::memcpy(&s, &bits, 8);
  const double z = std::fma(c[6], r, c[7]);
  const double y = std::fma(z, r*r, std::fma(c[8], r, 1.0));
  return static_cast<float>(y*s);
}

// log2f/log2f_data, same upstream release and independently checked ELF data.
extern "C" const double k1_probe_log2f_data[] = {
  0x1.661ec79f8f3bep+0, -0x1.efec65b963019p-2,
  0x1.571ed4aaf883dp+0, -0x1.b0b6832d4fca4p-2,
  0x1.49539f0f010bp+0, -0x1.7418b0a1fb77bp-2,
  0x1.3c995b0b80385p+0, -0x1.39de91a6dcf7bp-2,
  0x1.30d190c8864a5p+0, -0x1.01d9bf3f2b631p-2,
  0x1.25e227b0b8eap+0, -0x1.97c1d1b3b7afp-3,
  0x1.1bb4a4a1a343fp+0, -0x1.2f9e393af3c9fp-3,
  0x1.12358f08ae5bap+0, -0x1.960cbbf788d5cp-4,
  0x1.0953f419900a7p+0, -0x1.a6f9db6475fcep-5,
  0x1p+0, 0x0p+0,
  0x1.e608cfd9a47acp-1, 0x1.338ca9f24f53dp-4,
  0x1.ca4b31f026aap-1, 0x1.476a9543891bap-3,
  0x1.b2036576afce6p-1, 0x1.e840b4ac4e4d2p-3,
  0x1.9c2d163a1aa2dp-1, 0x1.40645f0c6651cp-2,
  0x1.886e6037841edp-1, 0x1.88e9c2c1b9ff8p-2,
  0x1.767dcf5534862p-1, 0x1.ce0a44eb17bccp-2,
  -0x1.712b6f70a7e4dp-2, 0x1.ecabf496832ep-2, -0x1.715479ffae3dep-1, 0x1.715475f35c8b8p0
};
extern "C" float log2f(float x) {
  // Tempo priors use positive normal inputs only. Do not generalise this probe.
  if (!std::isfinite(x) || x < 0x1p-126F) std::abort();
  if (x == 1.0F) return 0;
  std::uint32_t bits; std::memcpy(&bits, &x, 4);
  const auto tmp = bits-0x3f330000U;
  const unsigned i=(tmp>>19)&15;
  const int k=static_cast<std::int32_t>(tmp)>>23;
  bits -= tmp&0xff800000U;
  float z; std::memcpy(&z,&bits,4);
  const auto* t=k1_probe_log2f_data;
  const double r=std::fma(double(z),t[2*i],-1.0);
  const double r2=r*r, y0=t[2*i+1]+double(k);
  const double y=std::fma(t[32],r2,std::fma(t[33],r,t[34]));
  return static_cast<float>(std::fma(y,r2,std::fma(t[35],r,y0)));
}
