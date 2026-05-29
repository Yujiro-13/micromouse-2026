

u = duty_l;
y = vel_current;

np = 2;
nz = 0;

Ts = 0.001;

opt = tfestOptions('InitializeMethod', 'all', 'Display', 'on');

sys = tfest(u,y,np,nz,'Ts',Ts,opt)