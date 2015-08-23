using System;
using System.IO;
using System.Runtime.InteropServices;

namespace ImageDiff
{
  public struct Color
  {
    public byte R, G, B, A;
    public static Color FromArgb(byte A, byte R, byte G, byte B)
    {
      return new Color() { A = A, R = R, G = G, B = B };
    }

    public static readonly Color Red = Color.FromArgb(255, 255, 0, 0);
  }

  public class Bitmap : IDisposable
  {
    public int Width { get; private set; }
    public int Height { get; private set; }

    internal NativeImage nativeImage;

    public Bitmap(string filename)
    {
      nativeImage = Binding.LoadImage(filename);
      Width = nativeImage.Width;
      Height = nativeImage.Height;
    }

    public Bitmap(Stream stream)
    {
      byte[] buffer = new byte[16 * 1024];
      byte[] allBytes;

      using (MemoryStream ms = new MemoryStream())
      {
        int read;
        while ((read = stream.Read(buffer, 0, buffer.Length)) > 0)
        {
          ms.Write(buffer, 0, read);
        }

        allBytes = ms.ToArray();
      }

      nativeImage = Binding.LoadImage(allBytes, allBytes.Length);
      Width = nativeImage.Width;
      Height = nativeImage.Height;
    }

    public Bitmap(int width, int height, IntPtr data)
    {
      Width = width;
      Height = height;
      nativeImage = new NativeImage() { Width = width, Height = height, Data = data };
    }

    public void Save(string filename)
    {
      Binding.SavePng(nativeImage, filename);
    }

    public void Dispose()
    {
      if (IntPtr.Zero != nativeImage.Data)
      {
        Binding.FreeImgMem(nativeImage.Data);
      }
    }
  }

  public class Image
  {
    public static Bitmap FromFile(string filename)
    {
      return new Bitmap(filename);
    }

    public static Bitmap FromStream(Stream stream)
    {
      return new Bitmap(stream);
    }
  }
    

  [StructLayout(LayoutKind.Sequential)]
  internal struct NativeImage
  {
    public int Width;
    public int Height;
    public IntPtr Data;
  }

  [StructLayout(LayoutKind.Sequential)]
  internal struct NativeColor
  {
    byte B, G, R, A;
    public static NativeColor FromManaged(Color color)
    {
      NativeColor result = new NativeColor();
      result.A = color.A;
      result.R = color.R;
      result.G = color.G;
      result.B = color.B;
      return result;
    }
  }

  [StructLayout(LayoutKind.Sequential)]
  internal struct NativeDiffResult
  {
    public NativeImage Image;
    public float Similarity;
  }

  public class DiffResult : IDisposable
  {
    public Bitmap Image;
    public float Similarity;

    public void Dispose()
    {
      if(Image != null)
      {
        Image.Dispose();
      }
    }
  }

  [StructLayout(LayoutKind.Sequential, Pack = 1)]
  internal struct NativeDiffOptions
  {
    public NativeColor ErrorColor;
    public float Tolerance;
    public float OverlayTransparency;
    public OverlayType OverlayType;
    public byte WeightByDiffPercentage;
    public byte IgnoreColor;

    public static NativeDiffOptions FromManaged(DiffOptions options)
    {
      NativeDiffOptions result = new NativeDiffOptions();
      result.ErrorColor = NativeColor.FromManaged(options.ErrorColor);
      result.Tolerance = options.Tolerance;
      result.OverlayTransparency = options.OverlayTransparency;
      result.OverlayType = options.OverlayType;
      result.WeightByDiffPercentage = (byte)(options.WeightByDiffPercentage ? 1 : 0);
      result.IgnoreColor =  (byte)(options.IgnoreColor ? 1 : 0);
      return result;
    }
  }

  public enum OverlayType
  {
    Flat,
    Movement
  }

  public class DiffOptions
  {
    public Color ErrorColor = Color.Red;
    public float Tolerance = 0.0f;
    public float OverlayTransparency = 1.0f;
    public OverlayType OverlayType = OverlayType.Flat;
    public bool WeightByDiffPercentage = false;
    public bool IgnoreColor = false;
  }

  public static class Binding
  {
    [DllImport("imgdiff.dll", EntryPoint = "diff_img_byte")]
    private static extern NativeDiffResult DiffARGB(NativeImage left, NativeImage right, NativeDiffOptions options);

    [DllImport("imgdiff.dll", EntryPoint = "free_img_mem")]
    internal static extern void FreeImgMem(IntPtr ptr);

    [DllImport("imgdiff.dll", EntryPoint = "load_img", CharSet = CharSet.Ansi)]
    internal static extern NativeImage LoadImage(string filename);

    [DllImport("imgdiff.dll", EntryPoint = "load_img_mem", CharSet = CharSet.Ansi)]
    internal static extern NativeImage LoadImage(byte[] data, int len);

    [DllImport("imgdiff.dll", EntryPoint = "save_png", CharSet = CharSet.Ansi)]
    internal static extern void SavePng(NativeImage image, string filename);

    public static DiffResult Diff(Bitmap left, Bitmap right, DiffOptions options)
    {
      DiffResult result = new DiffResult();

      NativeDiffResult nr = DiffARGB(left.nativeImage, right.nativeImage, NativeDiffOptions.FromManaged(options));

      result.Similarity = nr.Similarity;
      result.Image = new Bitmap(nr.Image.Width, nr.Image.Height, nr.Image.Data);
      return result;
    }

  }
}
