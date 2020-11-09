function [c,obj,y] = k_means_clustering(X, c0, T)
    [m0, d0] = size(X);
    [mc, dc] = size(c0);
    
    c = zeros(size(c0));
    y = zeros([m0,1]);
    obj = 0;
    ccheck = c0;
    
    for t = 1:T
        ctemp = zeros([mc,dc]);
        cdiv = zeros([mc, 1]);
        for i = 1:m0
            min_dist = -1;
            c_val = 0;
            for j = 1:mc
                p_dist = get_dist(X(i, :), ccheck(j, :));
                if((p_dist < min_dist)||(min_dist < 0))
                    min_dist = p_dist;
                    c_val = j;
                end
            end
            ctemp(c_val,:) = ctemp(c_val,:) + X(i,:);
            cdiv(c_val) = cdiv(c_val) + 1;
            y(i) = c_val;
        end
        c = c + (ctemp./cdiv);
        ccheck = ctemp./cdiv;
    end
    c = c./T;
    for curr=1:m0
        d = -1;
        for k = 1:mc
            curr_dist = dist(X(curr), c(k));
            if((curr_dist < d)||(d < 0))
                d = curr_dist;
            end
        end
        obj = obj + d;
    end
    
end

function [dist] = get_dist(p0,p1)
    dist = sum(sqrt((p0 - p1).^2));
end

