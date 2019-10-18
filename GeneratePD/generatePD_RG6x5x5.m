% generate descriptio and write to write

n=6;
fileID = fopen('PD_RG6x5x5.txt','w');
keep_mask = [];

% keyword 1
fprintf(fileID,'Random variables:\n');
A = permutation_induced_product_base(n,2);
[tmp,num_cols] = size(A);

rvs = [];
first = 1;
mask_length = length(mask);
for i=1:num_cols    
    if A{i}(1)~=A{i}(2)
        keep_mask = [keep_mask,i];
        if first 
            rvs = 'S';
            first = 0;
        else
            rvs = [rvs,',S'];
        end
        rvs = [rvs,sprintf('%d%d',A{i}(1),A{i}(2))];        
    end
   
end
fprintf(fileID,'%s', rvs);
fprintf(fileID,'\n\n');

% keyword 2
fprintf(fileID,'Additional LP variables:\n');
fprintf(fileID,'A,B\n\n');

% keyword 3
fprintf(fileID,'Objective:\n');
fprintf(fileID,'A+B\n\n');

% keyword 4
fprintf(fileID,'Dependency:\n');
for i=1:n
    start = 1;
    for j= keep_mask %1:num_cols
        %if A{j}(1)~=A{j}(2)
            if A{j}(1)==i
                if start
                    fprintf(fileID,'S%d%d',A{j}(1),A{j}(2));
                    start = 0;
                else
                    fprintf(fileID,',S%d%d',A{j}(1),A{j}(2));
                end
            end
        %end
    end
    fprintf(fileID,':');
    start = 1;
    for j=keep_mask% 1:num_cols
        %if A{j}(1)~=A{j}(2)
            if A{j}(2)==i
                if start
                    fprintf(fileID,'S%d%d',A{j}(1),A{j}(2));
                    start = 0;
                else
                    fprintf(fileID,',S%d%d',A{j}(1),A{j}(2));
                end
            end
        %end
    end
    fprintf(fileID,'\n');
end
fprintf(fileID,'\n');

% keyword 5
fprintf(fileID,'Constant bounds:\n');
fprintf(fileID,'H(S12,S13,S14,S15,S16)<=A\n');
fprintf(fileID,'H(S12)<=B\n');
fprintf(fileID,'H(%s)>=1\n\n',rvs);

% keyword 6
fprintf(fileID,'Symmetry:\n');

A = permutation_induced_product_tuples(n,2);
[num_rows,num_cols] = size(A);
for k=1:num_rows
    first = 1;
    for i = keep_mask %i=1:num_cols
        %if A{k,i}(1)~=A{k,i}(2)
            if first
                fprintf(fileID,'S');
                first = 0;
            else
                fprintf(fileID,',S');
            end
            fprintf(fileID,'%d%d',A{k,i}(1),A{k,i}(2));
        %end
    end    
    fprintf(fileID,'\n');
end
fprintf(fileID,'\n');

fprintf(fileID,'end\n');
fclose(fileID);
