function hueplot(thetas, confs)

if ~exist('confs', 'var')
  confs = ones(size(thetas));
end

img = hsv2rgb(mod(thetas, pi)/pi, confs, ones(size(confs)));
imshow(img);
