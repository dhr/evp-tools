function reduced = flowreduce(cols, thresh)

if ~exist('thresh', 'var')
  thresh = 0;
end

reduced = double(max(max(cols, [], 5), [], 4));
reduced(reduced < thresh) = 0;