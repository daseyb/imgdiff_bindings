using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Drawing;
using System.Drawing.Imaging;

namespace ImageDiff
{
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
    internal IntPtr RawData;

    public void Dispose()
    {
      if(IntPtr.Zero != RawData)
      {
        Binding.FreeImgMem(RawData);
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

    public static DiffResult Diff(Bitmap left, Bitmap right, DiffOptions options)
    {
      DiffResult result = new DiffResult();
      BitmapData leftData = left.LockBits(new Rectangle(0, 0, left.Width, left.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
      BitmapData rightData = right.LockBits(new Rectangle(0, 0, right.Width, right.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);

      NativeImage leftImg = new NativeImage() { Width = leftData.Width, Height = leftData.Height, Data = leftData.Scan0 };
      NativeImage rightImg = new NativeImage() { Width = rightData.Width, Height = rightData.Height, Data = rightData.Scan0 };

      NativeDiffResult nr = DiffARGB(leftImg, rightImg, NativeDiffOptions.FromManaged(options));
      left.UnlockBits(leftData);
      right.UnlockBits(rightData);

      result.Similarity = nr.Similarity;
      result.Image = new Bitmap(nr.Image.Width, nr.Image.Height, 4 * nr.Image.Width, PixelFormat.Format32bppArgb, nr.Image.Data);
      result.RawData = nr.Image.Data;
      return result;
    }

  }
}
