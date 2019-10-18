function A = permutation_induced_product_base(N,dim)

num_cols=1;
for i=1:dim
    num_cols = num_cols*N;
end

A = cell(1,num_cols);
for k=0:num_cols-1
    current_val = k;
    for i=1:dim
        A{1,k+1}(dim-i+1) = mod(current_val,N)+1;
        current_val = floor(current_val/N);
    end
end
