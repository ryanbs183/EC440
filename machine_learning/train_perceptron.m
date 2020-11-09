function [w, b, average_w, average_b] = train_perceptron(X,y)
    %Perceptron algorithm
    
    [col, rows] = size(X);
    
    wt = zeros([rows + 1,1]) - .5;
    
    for i = 1:col
        xi = X(i,:);
        yi = y(i,:);
        xt = [1; transpose(xi)];
        if yi*transpose(wt)*xt < 0
            wt = wt + yi*xt;
        end
    end
    
    w = wt(2:end);
    b = wt(1);
    average_w = mean(w);
    average_b = mean(b);
end

