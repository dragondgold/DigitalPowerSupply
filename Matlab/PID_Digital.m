time = [];
output = [];
measured = [];

setpoint = 512;
k1 = 2;
k2 = -2;
k3 = 0;

errorHistory = zeros(1, 3, 'int16')
prevOutput = 0;

for n=0 : 5000
    measuredOutput = sin(n*0.010)*50 + 800;
    error = int16(setpoint) - int16(measuredOutput);
    errorHistory(1) = int16(error);
    
    out = int16(errorHistory(1)*int16(k1) + errorHistory(2)*int16(k2) + errorHistory(3)*int16(k3) + int16(prevOutput));

    errorHistory(3) = int16(errorHistory(2));
    errorHistory(2) = int16(errorHistory(1));
    prevOutput = int16(out);
    
    time = [time n];
    output = [output int16(out)];
    measured = [measured uint16(measuredOutput)];
end

clf
figure(1)
hold on
plot(time, output)
plot(time, measured, '--')
hold off
