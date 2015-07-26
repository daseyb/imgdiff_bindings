using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ImageDiff;
using System.Drawing;
using System.Diagnostics;

namespace ImageDiffTest
{
  class Program
  {
    static void Main(string[] args)
    {
      string left = args[0];
      string right = args[1];
      string diff = args[2];

      int iterationCount = int.Parse(args[3]);

      Bitmap leftImg = new Bitmap(left);
      Bitmap rightImg = new Bitmap(right);
      DiffResult result = new DiffResult();
      Stopwatch perf = new Stopwatch();
      perf.Start();
      for (int i = 0; i < iterationCount; i++)
      {
        result.Dispose();
        result = ImageDiff.Binding.Diff(leftImg, rightImg,
          new DiffOptions()
          {
            ErrorColor = Color.FromArgb(255, 255, 0, 0),
            Tolerance = 0.2f,
            OverlayTransparency = 1.0f,
            OverlayType = OverlayType.Flat,
            WeightByDiffPercentage = false,
            IgnoreColor = true
          });
      }
      perf.Stop();
      double msPerIter = (double)perf.ElapsedMilliseconds/iterationCount;
      Console.WriteLine(msPerIter + " ms per iteration.");
      double msPerPixel = msPerIter / (result.Image.Width * result.Image.Height);
      Console.WriteLine(msPerPixel + " ms per pixel.");
      result.Image.Save(diff);
      result.Dispose();
      Console.ReadLine();
    }
  }
}
