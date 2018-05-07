# Image-Morphing

Image Morphing - Implemented using warp morphing technique, cross-dissolving and Bilinear \ Gaussian interpolations for color estimation.

The program should be run from command line in the following manner: <exe_file> <config_file>
Where <exe_file> is the name of the program's executable and <config file> is the name of a text
file in the following format:
  Line 1: source image file name
  Line 2: destination image file name
  line 3: source warp sequence folder name
  line 4: destination warp sequence folder name
  line 5: morph sequence folder name
  line 6: <sequence length> <a> <b> <p>

Following lines should contain pairs of corresponding lines in the source and destination
images, defined by their starting and ending points: <x_src> <y_src> <x_dst> <y_dst>
<a>,<b> and <p> are the coefficients that control the influence of each line on the warping.
The program outputs are 3 sequences of images, each sequence containing
<sequence length> images:
  1. The warping sequence of the source image, saved in the folder specified in line 1 of
  the config file.
  2. The warping sequence of the destination image, saved in the folder specified in
  line 2 of the config file.
  3. The morphing sequence, which is the blending (cross-dissolving) of the
  corresponding images of the warped sequences, weighed according to the value of t
  and <sequence length>.


Color estimation is calculated by Bilinear interpolation by default, and can be triggered to give better and smoother color estimation with '-g' flag added as a Bonus for Gaussian estimation using Gaussian values matrix. 
