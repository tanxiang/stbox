#include "header.cl"

__kernel void TEMPLATE(cll_backward,Dtype)(const int_tp count, const int_tp channels,
                            const Dtype margin, const Dtype alpha, __global const Dtype* y,
                            __global const Dtype* diff, __global const Dtype* dist_sq,
                            __global Dtype *bottom_diff) {
  for (int_tp i = get_global_id(0); i < count;
      i += get_global_size(0)) {
    int_tp n = i / channels;  // the num index, to access y and dist_sq
    if (trunc(y[n]) != 0.) {  // similar pairs
      bottom_diff[i] = alpha * diff[i];
    } else {  // dissimilar pairs
      Dtype dist = sqrt(dist_sq[n]);
      Dtype mdist = (margin - dist);
      if (mdist > 0.) {
        Dtype plus = 0.0001;
        bottom_diff[i] = -alpha * mdist / (dist + plus) * diff[i];
      } else {
        bottom_diff[i] = 0;
      }
    }
  }
}

__kernel void TEMPLATE(cll_backward_legacy,Dtype)(const int count, const int channels,
    const Dtype margin, const Dtype alpha, __global Dtype* y,
    __global Dtype* diff, __global Dtype* dist_sq,
    __global Dtype* bottom_diff) {
    for (int_tp i = get_global_id(0); i < count;
      i += get_global_size(0)) {
    int n = i / channels;  // the num index, to access y and dist_sq
    if (trunc(y[n]) != 0.) {  // similar pairs
      bottom_diff[i] = alpha * diff[i];
    } else {  // dissimilar pairs
      Dtype mdist = (margin - dist_sq[n]);
      if (mdist > 0.) {
        bottom_diff[i] = -alpha;
      } else {
        bottom_diff[i] = 0;
      }
    }
  }
}
