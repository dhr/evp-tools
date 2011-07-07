function img = imload(pathname)

if ~isempty(regexpi(pathname, '\.pgm$', 'once'))
  img = readpgm(pathname);
else
  img = imread(pathname);
end

switch class(img)
  case 'logical'
    maxval = 1;
  case 'uint8'
    maxval = 255;
  case 'uint16'
    maxval = 65535;
  case 'double'
    maxval = 1;
  otherwise
    error(strcat('Unexpected image format ', class(img), '.'));
end

img = double(img)/maxval;
img = sum(img, 3)/size(img, 3);

function image = readpgm(filename)
%READPGM Read a raw pgm file as a matrix
%   IMAGE = READPGM(FILENAME) reads the binary PGM image data from the file
%   named FILENAME and returns the image as a 2-dimensional array of
%   integers IMAGE. Assumes the file is a raw PGM file containing 8-bit
%   unsigned character data to represent pixel values.
%
%   Matthew Dailey, 1997

fid = fopen(filename, 'r');

% Parse and check the header information.  No # comments allowed.
A = fgets(fid);
if strcmp(A(1:2), 'P5') ~= 1
  fclose(fid);
  error('File is not a raw PGM.');
end;

A = fgets(fid);
sizes = sscanf(A, '%d');
w = sizes(1);
h = sizes(2);

A = fgets(fid);
max = sscanf(A, '%d');
tlength = w*h;

if max ~= 255
  fclose(fid);
  error('Cannot handle anything but 8-bit graymaps.');
end;

[v, count] = fread(fid, inf, 'uchar');
fclose(fid);

if count ~= tlength
  error('File size does not agree with specified dimensions.');
end;

image = reshape(v, w, h)';
