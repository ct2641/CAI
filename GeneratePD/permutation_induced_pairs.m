function A = permutation_induced_tuples(N,dim)
% generate the full permuted pairs 
num_cols=1;
num_rows=1;
for i=1:dim
    num_cols = num_cols*N;    
end

for i=1:N
    num_rows = num_rows*i;
end

A = cell(num_rows,num_cols);
permutation_list = perms(1:N); 

% make base tuples 

for i=1:num_rows
    current_perm = permutation_list(i,:);
    
end

