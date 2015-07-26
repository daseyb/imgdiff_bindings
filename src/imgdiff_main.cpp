#include <assert.h>
#include <stdint.h>
#include <math.h>

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
  Color* data;
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

inline BColor* ConvertToByte(uint32 w, uint32 h, Color* in) {
  BColor* data = new BColor[w *h];
  BColor* d = data;
  for (uint32 y = 0; y < h; y++) {
    for (uint32 x = 0; x < w; x++) {
      auto p = *in;
      *d = ARGB( byte(p.a * 255), byte(p.r * 255), byte(p.g * 255), byte(p.b * 255) );
      d++;
      in++;
    }
  }
  return data;
}

inline Color* ConvertToFloat(uint32 w, uint32 h, BColor* in) {
  Color* data = new Color[w *h];
  Color* d = data;
  for (uint32 y = 0; y < h; y++) {
    for (uint32 x = 0; x < w; x++) {
      auto p = *in;
      *d= ARGB(float(p.a) / 255, float(p.r) / 255, float(p.g) / 255, float(p.b) / 255);
      d++;
      in++;
    }
  }
  return data;
}

inline Image ConvertToFloat(BImage in) {
  Image inf = { in.width, in.height, nullptr };
  inf.data = ConvertToFloat(in.width, in.height, in.data);
  return inf;
}

inline BImage ConvertToByte(Image in) {
  BImage inb = { in.width, in.height, nullptr };
  inb.data = ConvertToByte(in.width, in.height, in.data);
  return inb;
}

inline bool operator==(const Color& a, const Color& b) {
  return a.a == b.a && a.r == b.r && a.g == b.g && a.b == b.b;
}

inline Color operator-(const Color& a, const Color& b) {
  return ARGB(a.a - b.a, a.r - b.r, a.g - b.g, a.b - b.b);
}

inline Color operator+(const Color& a, const Color& b) {
  return ARGB(a.a + b.a, a.r + b.r, a.g + b.g, a.b + b.b);
}

inline Color operator*(const Color& a, const Color& b) {
  return ARGB(a.a * b.a, a.r * b.r, a.g * b.g, a.b * b.b);
}

inline Color operator*(const Color& a, float mul) {
  return ARGB(a.a * mul, a.r * mul, a.g * mul, a.b * mul);
}

inline float clamp01(float v) {
  return v < 0 ? 0 : v > 1 ? 1 : v;
}

inline Color saturate(const Color& a) {
  return ARGB(clamp01(a.a) , clamp01(a.r), clamp01(a.g), clamp01(a.b));
}

inline Color lerp(const Color& a, const Color& b, float t) {
  t = clamp01(t);
  return a + (b-a) *t;
}

inline Color mix(const Color& a, const Color& b, float t) {
  t = clamp01(t);
  return a * (1.0f - t) + b * t;
}

inline Color ConvertToFloat(BColor col) {
  return ARGB(float(col.a) / 255, float(col.r) / 255, float(col.g) / 255, float(col.b) / 255);
}

inline void makeGreyscale(Image image) {
  Color* d = (Color*)image.data;

  for (int y = 0; y < image.height; y++) {
    for (int x = 0; x < image.width; x++) {
      Color col = *d;
      float b = 0.3f*col.r + 0.59f*col.g + 0.11f*col.b;
      *d = ARGB(col.a, b, b, b);
      d++;
    }
  }
}

extern "C" {
  _declspec(dllexport) DiffResult __stdcall diff_img(Image left, Image right, DiffOptions options) {
    assert(left.width == right.width && left.height == right.width);

    if (options.ignoreColor) {
      makeGreyscale(left);
      makeGreyscale(right);
    }

    Image diff = { left.width, left.height, new Color[left.width * left.height] };
    Color* d = (Color*)diff.data;
    Color* l = (Color*)left.data;
    Color* r = (Color*)right.data;
    Color error = ConvertToFloat(options.errorColor);
    float tolerance = options.tolerance;
    float overlayTransparency = options.overlayTransparency;
    OverlayType overlayType = options.overlayType;
    byte weightByDiffPercentage = options.weightByDiffPercentage;

    int similarPixelCount = 0;
    for (int y = 0; y < left.height; y++) {
      for (int x = 0; x < left.width; x++) {
        Color left = *l;
        Color right = *r;
        Color diff = right - left;

        float distance = sqrtf(diff.r * diff.r +
          diff.g * diff.g +
          diff.b * diff.b +
          diff.a * diff.a);

        float t = overlayTransparency;
        if (weightByDiffPercentage) {
          t *= distance;
        }

        bool isSimilar = distance <= tolerance;
        if (isSimilar) {
          *d = left;
        } else {
          if (overlayType == OverlayType::Movement) {
            *d = isSimilar ? left : mix(right * error, error, t);
            d->a = 1;
          } else {
            *d = isSimilar ? left : mix(right, error, t);
          }
        }

        if (isSimilar) {
          similarPixelCount++;
        }

        d++;
        l++;
        r++;
      }
    }

    return{ diff, float(similarPixelCount) / (left.height * left.width) };
  }

  _declspec(dllexport) BDiffResult __stdcall diff_img_byte(BImage left, BImage right, DiffOptions options) {
    Image leftf = ConvertToFloat(left);
    Image rightf = ConvertToFloat(right);

    DiffResult resultf = diff_img(leftf, rightf, options);
    BDiffResult resultb = { ConvertToByte(resultf.img), resultf.similarity };
    delete(resultf.img.data);
    delete(leftf.data);
    delete(rightf.data);
    return resultb;
  }
}




