This is for Alienware Area 51m r2 with rx5700m only !!!!

This repo will fix the backlight adjust problem of this machine, see the code for more detail.

Still have two problem:

First, dim screen when on battery seems not working.

Second, the backlight control does not work when cold boot into Mac OS, and it will not going to work until boot into Windows and then warm boot to Mac OS. Don't know the reason, may be need more initialize or reset code. And the most interesting thing is, if you boot into Ubuntu when this problem occurs, it will show some similiar errors like this.

![image](https://user-images.githubusercontent.com/46492291/132368573-15901d6a-8b5e-446b-b66d-0f7c0cf0eb18.png)

No clue how to fix this. Please help if you have any ideas!

# Related issue

* https://bugzilla.kernel.org/show_bug.cgi?id=203905
