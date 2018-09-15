#### Read FAQ and discussion on external links:  
[Russian](https://www.applelife.ru/threads/intel-hd-graphics-3000-4000-4400-4600-5000-5500-5600-520-530-630.1289648/)  
[English](https://www.insanelymac.com/forum/topic/334899-intel-framebuffer-patching-using-whatevergreen/)  

AzulPatcher4600.kext deprecated because it is equivalent to use `framebuffer-patch` in `Devices-Properties`  
For example, patch fCursorMemorySize=9MB:  
`framebuffer-patch-enable` = `01000000`  
`framebuffer-cursormem` = `00009000`  
Read FAQ in more detail.  
