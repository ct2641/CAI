function A = permutation_induced_product_tuples(N,dim)
% generate the full permuted tuples 

num_rows=1;
num_cols=1;
for i=1:dim
    num_cols = num_cols*N;
end
for i=1:N
    num_rows = num_rows*i;
end

A = cell(num_rows,num_cols);

A(1,:) = permutation_induced_product_base(N,dim);
permutation_list = perms(1:N); 

for i=1:num_rows
    if isequal(permutation_list(i,:),1:N) 
        permutation_list = permutation_list([i,1:i-1,i+1:end],:);
        break;
    end
end

for i=1:num_rows
    current_perm = permutation_list(i,:);
    for j=1:num_cols
        for k=1:dim
            A{i,j}(k)=current_perm(A{1,j}(k));
        end
    end        
end

