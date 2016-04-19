# imgdiff
Fast image comparison and diffing in C++ (with a C# binding)

Inspired by [resemble.js](https://huddle.github.io/Resemble.js/)

Basically exactly the same functionality, just not JS. (i.e. faster, uses SSE2, small memory footprint)

Build bindings\ImageDiff for an easy to use C# binding.

### Example Code
    Bitmap leftImg = new Bitmap(leftFile);
    Bitmap rightImg = new Bitmap(rightFile);
    using (DiffResult result = ImageDiff.Binding.Diff(leftImg, rightImg,
          new DiffOptions()
          {
            ErrorColor = Color.FromArgb(255, 255, 0, 255),
            Tolerance = 0.2f,
            OverlayTransparency = 1.0f,
            OverlayType = OverlayType.Movement,
            WeightByDiffPercentage = false,
            IgnoreColor = false
          }))
    {
      result.Image.Save(diffFile);
      Console.WriteLine(result.Similarity);
    }
    
### Result
### Input:
_________________
![Left](https://raw.githubusercontent.com/bonus2113/imgdiff_bindings/master/data/1_normal.jpg)  ![Right](https://raw.githubusercontent.com/bonus2113/imgdiff_bindings/master/data/1_modified.jpg)

### Diff:
_________________
![Diff](https://raw.githubusercontent.com/bonus2113/imgdiff_bindings/master/data/1_diff.jpg)
