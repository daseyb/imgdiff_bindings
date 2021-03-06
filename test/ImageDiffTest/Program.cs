﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ImageDiff;
using System.Diagnostics;
using System.IO;

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

      FileStream leftStream = File.OpenRead(left);
      FileStream rightStream = File.OpenRead(right);

      Bitmap leftImg = new Bitmap(leftStream);
      Bitmap rightImg = new Bitmap(rightStream);

      DiffResult result = new DiffResult();
      Stopwatch perf = new Stopwatch();
      perf.Start();
      for (int i = 0; i < iterationCount; i++)
      {
        result.Dispose();
        result = ImageDiff.Binding.Diff(leftImg, rightImg,
          new DiffOptions()
          {
            ErrorColor = Color.FromArgb(255, 255, 0, 255),
            Tolerance = 0.15f,
            OverlayTransparency = 1.0f,
            OverlayType = OverlayType.Movement,
            WeightByDiffPercentage = false,
            IgnoreColor = false
          });
      }
      perf.Stop();
      double msPerIter = (double)perf.ElapsedMilliseconds / iterationCount;
      Console.WriteLine(msPerIter + " ms per iteration.");
      double msPerPixel = msPerIter / (result.Image.Width * result.Image.Height);
      Console.WriteLine(msPerPixel * 1000 + " ns per pixel.");
      result.Image.Save(diff);
      result.Dispose();
    }
  }
}


