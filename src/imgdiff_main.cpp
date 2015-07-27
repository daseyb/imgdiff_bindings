#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <mmintrin.h>
#include <intrin.h>

typedef uint8_t byte;
typedef uint32_t uint32;

template<typename T>
struct TColor {
  T b, g, r, a;
};

typedef TColor<float> Color;
typedef TColor<byte> BColor;

struct BImage {
  int width;
  int height;
  BColor* data;
};

struct Image {
  int width;
  int height;
  int stride;

  float* r;
  float* g;
  float* b;
  float* a;
};

struct BDiffResult {
  BImage img;
  float similarity;
};

enum OverlayType {
  Flat,
  Movement
};

#pragma pack(push, 1)
struct DiffOptions {
  BColor errorColor;
  float tolerance;
  float overlayTransparency;
  OverlayType overlayType;
  byte weightByDiffPercentage;
  byte ignoreColor;
};
#pragma pack(pop)

struct DiffResult {
  Image img;
  float similarity;
};

template<typename T>
inline TColor<T> ARGB(T a, T r, T g, T b) {
  TColor<T> result;
  result.a = a;
  result.r = r;
  result.g = g;
  result.b = b;
  return result;
}

struct alignas(16) simd4 {
  float v[4];
};

inline BColor* ConvertToByte(uint32 w, uint32 h, int stride, float* r, float* g, float* b, float* a) {
  w -= stride;
  BColor* data = (BColor*)malloc(w*h*sizeof(BColor));
  BColor* d = data;
  for (uint32 y = 0; y < h; y++) {
    for (uint32 x = 0; x < w; x++) {
      *d = ARGB(byte( (*a) * 255), byte( (*r) * 255), byte( (*g) * 255), byte( (*b) * 255));
      d++;
      r++;
      g++;
      b++;
      a++;
    }
    r += stride;
    g += stride;
    b += stride;
    a += stride;
  }
  return data;
}

inline Image ConvertToFloat(BImage in) {
  int w = in.width;
  int h = in.height;

  BColor* inData = in.data;
  
  int stride = (4 - (w % 4)) % 4;

  w += stride;

  assert(w % 4 == 0);

  float* rBase = (float*)_aligned_malloc(w * h * sizeof(float), 16);
  float* gBase = (float*)_aligned_malloc(w * h * sizeof(float), 16);
  float* bBase = (float*)_aligned_malloc(w * h * sizeof(float), 16);
  float* aBase = (float*)_aligned_malloc(w * h * sizeof(float), 16);

  float* r = rBase;
  float* g = gBase;
  float* b = bBase;
  float* a = aBase;

  Image inf = { w, h, stride, rBase, gBase, bBase, aBase };

  w -= stride;
  for (uint32 y = 0; y < h; y++) {
    for (uint32 x = 0; x < w; x++) {
      auto p = *inData;

      *a = float(p.a) / 255;
      *r = float(p.r) / 255;
      *g = float(p.g) / 255;
      *b = float(p.b) / 255;
      
      a++;
      r++;
      g++;
      b++;
      inData++;
    }

    for (int i = 0; i < stride; i++) {
      *a++ = 0;
      *r++ = 0;
      *g++ = 0;
      *b++ = 0;
    }
  }

  return inf;
}

inline BImage ConvertToByte(Image in) {
  BImage inb = { in.width, in.height, nullptr };
  inb.data = ConvertToByte(in.width, in.height, in.stride, in.r, in.g, in.b, in.a);
  return inb;
}

inline Color ConvertToFloat(BColor col) {
  return ARGB(float(col.a) / 255, float(col.r) / 255, float(col.g) / 255, float(col.b) / 255);
}

inline void makeGreyscale(Image image) {
  float* r = image.r;
  float* g = image.g;
  float* b = image.b;

  auto rf = _mm_set1_ps(0.3f);
  auto gf = _mm_set1_ps(0.59f);
  auto bf = _mm_set1_ps(0.11f);

  for (int y = 0; y < image.height; y++) {
    for (int x = 0; x < image.width; x+=4) {
      auto rs = _mm_load_ps(r);
      auto gs = _mm_load_ps(g);
      auto bs = _mm_load_ps(b);

      auto br = _mm_mul_ps(rs, rf);
      br = _mm_add_ps(br, _mm_mul_ps(gs, gf));
      br = _mm_add_ps(br, _mm_mul_ps(bs, bf));
      _mm_store_ps(r, br);
      _mm_store_ps(g, br);
      _mm_store_ps(b, br);
      r+=4;
      g+=4;
      b+=4;
    }
  }
}


extern "C" {
  _declspec(dllexport) DiffResult __stdcall diff_img(Image left, Image right, DiffOptions options) {
    assert(left.width == right.width && left.height == right.height);

    if (options.ignoreColor) {
      makeGreyscale(left);
      makeGreyscale(right);
    }

    Image diff = { left.width, left.height, left.stride,
      (float*)_aligned_malloc(left.width * left.height * sizeof(float), 16),
      (float*)_aligned_malloc(left.width * left.height * sizeof(float), 16),
      (float*)_aligned_malloc(left.width * left.height * sizeof(float), 16),
      (float*)_aligned_malloc(left.width * left.height * sizeof(float), 16) };

    float* drp = diff.r;
    float* dgp = diff.g;
    float* dbp = diff.b;
    float* dap = diff.a;

    float* lrp = left.r;
    float* lgp = left.g;
    float* lbp = left.b;
    float* lap = left.a;

    float* rrp = right.r;
    float* rgp = right.g;
    float* rbp = right.b;
    float* rap = right.a;

    Color error = ConvertToFloat(options.errorColor);

    auto er = _mm_set_ps1(error.r);
    auto eg = _mm_set_ps1(error.g);
    auto eb = _mm_set_ps1(error.b);
    auto ea = _mm_set_ps1(error.a);

    auto tolerance = _mm_set_ps1(options.tolerance);
    auto overlayTransparency = _mm_set_ps1(options.overlayTransparency);

    OverlayType overlayType = options.overlayType;
    byte weightByDiffPercentage = options.weightByDiffPercentage;

    auto diffPixelCount = _mm_set_epi32(0, 0, 0, 0);
    auto onei = _mm_set1_epi32(1);
    auto one = _mm_set1_ps(1);
    auto zero = _mm_set1_ps(0);

    for (int y = 0; y < left.height; y++) {
      for (int x = 0; x < left.width; x+=4) {
        auto lr = _mm_load_ps(lrp);
        auto lg = _mm_load_ps(lgp);
        auto lb = _mm_load_ps(lbp);
        auto la = _mm_load_ps(lap);

        auto rr = _mm_load_ps(rrp);
        auto rg = _mm_load_ps(rgp);
        auto rb = _mm_load_ps(rbp);
        auto ra = _mm_load_ps(rap);

        auto rdiff = _mm_sub_ps(rr, lr);
        auto gdiff = _mm_sub_ps(rg, lg);
        auto bdiff = _mm_sub_ps(rb, lb);
        auto adiff = _mm_sub_ps(ra, la);

        auto distance = _mm_mul_ps(rdiff, rdiff);
        distance = _mm_add_ps(distance, _mm_mul_ps(gdiff, gdiff));
        distance = _mm_add_ps(distance, _mm_mul_ps(bdiff, bdiff));
        distance = _mm_add_ps(distance, _mm_mul_ps(adiff, adiff));
        distance = _mm_sqrt_ps(distance);

        auto t = overlayTransparency;
        if (weightByDiffPercentage) {
          t = _mm_mul_ps(t, distance);
        }

        auto isdiff = _mm_cmpgt_ps(distance, tolerance);
        t = _mm_max_ps(one, _mm_min_ps(zero, t));
        auto mlr = rr;
        auto mlg = rg;
        auto mlb = rb;
        auto mla = ra;

        if (overlayType == OverlayType::Movement) {
          mlr = _mm_mul_ps(mlr, er);
          mlg = _mm_mul_ps(mlg, eg);
          mlb = _mm_mul_ps(mlb, eb);
          mla = _mm_mul_ps(mla, ea);
        }

        auto oneMinusT = _mm_sub_ps(one, t);

        auto mixedR = _mm_add_ps(_mm_mul_ps(mlr, oneMinusT), _mm_mul_ps(er, t));
        auto mixedG = _mm_add_ps(_mm_mul_ps(mlr, oneMinusT), _mm_mul_ps(eg, t));
        auto mixedB = _mm_add_ps(_mm_mul_ps(mlb, oneMinusT), _mm_mul_ps(eb, t));
        auto mixedA = one;

        if (overlayType != OverlayType::Movement) {
          mixedA = _mm_add_ps(_mm_mul_ps(mla, oneMinusT), _mm_mul_ps(ea, t));
        }

        // (((b ^ a) & mask)^a)
        auto dr = _mm_xor_ps(lr, _mm_and_ps(isdiff, _mm_xor_ps(mixedR, lr)));
        auto dg = _mm_xor_ps(lg, _mm_and_ps(isdiff, _mm_xor_ps(mixedG, lg)));
        auto db = _mm_xor_ps(lb, _mm_and_ps(isdiff, _mm_xor_ps(mixedB, lb)));
        auto da = _mm_xor_ps(la, _mm_and_ps(isdiff, _mm_xor_ps(mixedA, la)));

        diffPixelCount = _mm_xor_si128(diffPixelCount,
          _mm_and_si128(_mm_castps_si128(isdiff),
            _mm_xor_si128(_mm_add_epi32(diffPixelCount, onei),
              diffPixelCount)));

        _mm_store_ps(drp, dr);
        _mm_store_ps(dgp, dg);
        _mm_store_ps(dbp, db);
        _mm_store_ps(dap, da);

        drp+=4;
        dgp+=4;
        dbp+=4;
        dap+=4;

        lrp+=4;
        lgp+=4;
        lbp+=4;
        lap+=4;

        rrp+=4;
        rgp+=4;
        rbp+=4;
        rap+=4;
      }
    }

    alignas(16) int pixelCounts[4];
    _mm_store_si128((__m128i*)pixelCounts, diffPixelCount);

    int totalCount = pixelCounts[0] + pixelCounts[1] + pixelCounts[2] + pixelCounts[3];

    return{ diff, 1.0f - float(totalCount) / (left.height * left.width - left.height * left.stride) };
  }

  _declspec(dllexport) BDiffResult __stdcall diff_img_byte(BImage left, BImage right, DiffOptions options) {
    Image leftf = ConvertToFloat(left);
    Image rightf = ConvertToFloat(right);

    DiffResult resultf = diff_img(leftf, rightf, options);
    BDiffResult resultb = { ConvertToByte(resultf.img), resultf.similarity };

    _aligned_free(resultf.img.r);
    _aligned_free(resultf.img.g);
    _aligned_free(resultf.img.b);
    _aligned_free(resultf.img.a);

    _aligned_free(leftf.r);
    _aligned_free(leftf.g);
    _aligned_free(leftf.b);
    _aligned_free(leftf.a);

    _aligned_free(rightf.r);
    _aligned_free(rightf.g);
    _aligned_free(rightf.b);
    _aligned_free(rightf.a);

    return resultb;
  }

  _declspec(dllexport) void __stdcall free_img_mem(void* ptr) {
    free(ptr);
  }
}


int main(void) {
  BImage img1 = { 15, 15, (BColor*)malloc(15 * 15 * sizeof(BColor)) };
  BImage img2 = { 15, 15, (BColor*)malloc(15 * 15 * sizeof(BColor)) };
  
  for (int i = 0; i < 15 * 15; i++) {
    img1.data[i] = ARGB((byte)255, (byte)255, (byte)255, (byte)0);
    img2.data[i] = ARGB((byte)255, (byte)255, (byte)255, (byte)0);
  }

  auto diffResult = diff_img_byte(img1, img2, DiffOptions{ ARGB((byte)255, (byte)0, (byte)0, (byte)255), 0.2f, 1.0f, OverlayType::Flat, 0, 0 });

  free_img_mem(diffResult.img.data);

}



