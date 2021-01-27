
问题一：黑色横纹,亮度不够

![avatar](images/error1.png)

部分直接光由于精度问题，被错误判断为遮挡。通过精度调整放宽相交限制。

原使用的精度为 const float EPSILON = 0.00001; 

修改后 
![avatar](images/101.png)