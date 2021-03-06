% Parametros calculados del PID
Kp = 5
Ki = 6000
Kd = 5e-6

% Frecuencia de muestreo del ADC del PIC
Fs = 150.15e3    % 150.15 KSPS

% Creamos PID
G_discrete = pid(Kp,Ki,Kd,'Ts',1/Fs,'IFormula','Trapezoidal','DFormula','BackwardEuler')

% Discretizamos el sistema
[a,b,c,d] = linmod('SimulinkPID');
sys = ss(a,b,c,d);
tf(sys)
H_discrete = c2d(H, 1/Fs, 'zoh')

% Creamos el sistema de lazo cerrado siendo G la planta
T_pid = feedback(G_discrete*H_discrete, 1)
Fc = bandwidth(T_pid)/(2*pi)

% Graficamos la respuesta del sistema con lazo cerrado
figure(1);
step(T_pid)
grid on
title('Respuesta transitoria en lazo cerrado (plano discreto)')
xlabel('Tiempo')
ylabel('Amplitud')

% Graficamos la respuesta del sistema con lazo cerrado observando un
%  tiempo mayor para ver la respuesta en estado estacionario
figure(2);
step(T_pid, 0:1/Fs:1e-3)
grid on
title('Estado estacionario en lazo cerrado (plano discreto)')
xlabel('Tiempo')
ylabel('Amplitud')

% Graficamos la respuesta en frecuencia con lazo cerrado
optsV = bodeoptions('cstprefs');
optsV.FreqUnits = 'Hz';
optsV.Grid = 'on';

figure(3);
bode(T_pid, optsV)
grid on
title('Respuesta en frecuencia en lado cerrado (plano discreto)')
xlabel('Frecuencia')
ylabel('Amplitud')

% Diagrama de polos y ceros con lazo cerrado
figure(4);
zplane(zero(H*G), pole(H*G))
grid on
title('Diagrama de polos y ceros en lazo cerrado (plano discreto)')
axis equal

% Diagrama de Nyquist
figure(5)
nyquist(T_pid)
grid on
title('Diagrama de Nyquist en lazo cerrado (plano discreto)')
axis equal