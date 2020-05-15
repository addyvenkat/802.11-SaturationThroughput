clc;
clear;
close all;

%% Parameters

Payload = 8184; %bits
PHY_hdr = 128;  %bits
MAC_hdr = 272;  %bits
ACK_len = 112 + PHY_hdr;    %bits
SIFS = 28;  %us
DIFS = 128; %us
Slot_time = 50; %us
C_btrt = 1;    %bits/us
Tc = (((PHY_hdr+MAC_hdr+Payload)/C_btrt)+(DIFS/C_btrt))/Slot_time; %Slot time
Ts = (((PHY_hdr+MAC_hdr+Payload)/C_btrt)+(SIFS/C_btrt)+(ACK_len/C_btrt)+(DIFS/C_btrt))/Slot_time; %Slot time

%% Theoretical Computation

sims = input('# of simulations for analysis:\n');

W = [];
M = [];

for i = 1:sims
    W = [W,input('Min Backoff Window (in Slot time):\n')];
    M = [M,input('Max stage:\n')];
end

for i = 1:sims
    w = W(i);
    m = M(i);
    throughput = [];

    for n = 2:50
        func = @(x) (x - 1 + (1 - (2*(1-2*x))/((1-2*x)*(w+1) + x*w*(1-(2*x)^m)))^(n-1));
        p = fzero(func,[0,1]);
        T = (2*(1-2*p))/((1-2*p)*(w+1) + p*w*(1-(2*p)^m));
        Ptr = 1-(1-T)^n;
        Ps = n*T*(1-T)^(n-1)/Ptr;
        E_si = (1/Ptr) - 1;
        S = (Ps*(Payload/Slot_time))/(E_si + Ps*Ts + (1-Ps)*Tc);
        throughput = [throughput,S];
    end

    plot(2:50,throughput,'LineWidth',1.5,'DisplayName',['W=' num2str(w) ', m=' num2str(m)]);
    hold on;
end
title('Normalized Saturation throughput: Matlab')
xlabel('Number of Stations')
ylabel('Saturation Throughput')
legend
hold off;