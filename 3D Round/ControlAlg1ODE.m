[t,x] = ode45(@f2,[0,3],[10,-10]);

acc = diff(x(:,2))./diff(t);
x(2:end,3)=acc;
plot(t,x);
xlabel('Time (s)');
legend('position','velocity','acceleration');


function dxdt = f2(t,x)
dxdt(1,1) = x(2);
dxdt(2,1) = -sign(x(2))*(1+randn*0.15)*((x(2)^2)/(2*(abs(x(1)))));
end

function dxdt = f1(t,x)
dxdt(1,1) = x(2);
dxdt(2,1) = -(1+randn*0.15)*((x(2)^2)/(2*((x(1)))));
end