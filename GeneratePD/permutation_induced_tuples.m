function A = permutation_induced_tuples(N,dim)
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
for k=0:num_cols-1
    current_val = k;
    for i=1:dim
        A{1,k+1}(dim-i+1) = mod(current_val,N)+1;
        current_val = floor(current_val/N);
    end
end

permutation_list = perms(1:N); 
for i=2:num_rows
    current_perm = permutation_list(i,:);
    for j=1:num_cols
        for k=1:dim
            A{i,j}(k)=current_perm(A{1,j}(k));
        end
    end        
end

