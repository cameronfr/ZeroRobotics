[t,x] = ode45(@f2,[0,10],[10,-10]);

acc = diff(x(:,2))./diff(t);
x(2:end,3)=min(10,max(-10,acc));


plot(t,x);
xlabel('Time (s)');
legend('position','velocity','acceleration');


function dxdt = f2(t,x)
dxdt(1,1) = x(2);
if(sign(x(1)) == sign(x(2)))
    dxdt(2,1) = -3*sign(x(1));
else
    accel = -1.2*sign(x(2))*(1+randn*0.15)*((x(2)^2)/(2*(abs(x(1)))));
    %if (accel < 0)
    %    accel = max(-5, accel);
    %else
    %    accel = min(5,accel);
    %end
    dxdt(2,1) = accel;
end
end

function dxdt = f1(t,x)
dxdt(1,1) = x(2);
dxdt(2,1) = -(1+randn*0.15)*((x(2)^2)/(2*((x(1)))));
end