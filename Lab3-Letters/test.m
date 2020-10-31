clc 
clear all
close all
data = load('TPR_FPR.txt');
data1 = load('TP_FP.txt');
TP = data1(:,2);
FP = data1(:,3);
TPR = data(:,1);
FPR = data(:,2);

for i=1:length(FPR)
    dist(i) = sqrt((TPR(i) - 1)^2 + (FPR(i) - 0)^2);
end

mindist = min(dist);
index = find(mindist == dist);
TPR_min = TPR(index);
FPR_min = FPR(index);
TP_min = TP(index);
FP_min = FP(index);
Actual_Index = index-1;

figure()
plot(FPR, TPR,'linewidth',2.5,'color','black');
xlabel('False Positive Rate')
ylabel('True Positive Rate')
title('ROC Curve from Threshold: 0-255')

figure()
plot(FPR(171:221), TPR(171:221),'linewidth',2.5,'color','black');
hold on
plot(FPR(index), TPR(index), 'square','MarkerSize',11,'MarkerEdgeColor','black', 'MarkerFaceColor','red')
xlabel('False Positive Rate')
ylabel('True Positive Rate')
title('ROC Curve from Threshold: 170-220')
% xlim([0 1]);
% ylim([0 1]);
textspec2='Knee of Curve';
Text_on_plot2=sprintf(textspec2);
text(FPR(index)+0.01,TPR(index)-0.01,Text_on_plot2,'color','black','FontSize',10,'FontWeight','bold')

textspec23='FPR %.4f, TPR %.4f';
Text_on_plot23=sprintf(textspec23,FPR(index),TPR(index));
text(FPR(index)+0.01,TPR(index)-0.02,Text_on_plot23,'color','black','FontSize',10,'FontWeight','bold')