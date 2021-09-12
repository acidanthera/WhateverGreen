This is for Alienware Area 51m r2 with rx5700m only !!!!

This repo will fix the backlight adjust problem of this machine, see the code for more detail.

Still have two problem:

First, dim screen when on battery seems not work.

Second, the backlight control doesn't work when cold boot directly into Mac OS, and it will not going to work until boot into Windows/Ubuntu and then warm boot into Mac OS. Don't know the reason, may be need more initialize or reset code. And the most interesting thing is, if you boot into Ubuntu when this problem occurs, it will show some similiar errors like this.

![image](https://user-images.githubusercontent.com/46492291/132368573-15901d6a-8b5e-446b-b66d-0f7c0cf0eb18.png)

No clue how to fix this. Please help if you have any ideas!

May be we can look into this function in AMDRadeonX6000Framebuffer.kext for more information

```
__int64 __fastcall AmdDalServices::initialize(AmdDalServices *this, unsigned __int8 a2, __int64 a3)
{
  unsigned int (__fastcall *v3)(_QWORD, const char *, int *, signed __int64); // rax
  AmdDalServices *v4; // r12
  __int64 v5; // rax
  __int64 v6; // rax
  __int64 v7; // rdx
  __int64 v8; // rdi
  AmdDalDmcubService *v9; // rax
  __int64 v10; // rdi
  __int64 v11; // rax
  __int64 v12; // rbx
  __int64 v13; // rsi
  unsigned int v14; // ebx
  unsigned int v15; // eax
  unsigned int v16; // ebx
  AmdDalPsrService *v17; // rax
  const char *v18; // rsi
  const char *v19; // rdi
  AmdDalServices *v21; // [rsp+0h] [rbp-40h]
  __int64 v22; // [rsp+8h] [rbp-38h]
  __int64 v23; // [rsp+10h] [rbp-30h]
  int v24; // [rsp+1Ch] [rbp-24h]

  v24 = 0;
  v3 = (unsigned int (__fastcall *)(_QWORD, const char *, int *, signed __int64))*((_QWORD *)this + 83);
  if ( !v3 )
  {
    kprintf("ASSERT FAILED : %s.\n", "nullptr != m_cgsDevice.services.getProperty", a3);
    kprintf(
      "ASSERT LOCATION : %s :: %s:%u .\n",
      "virtual IOReturn AmdDalServices::initialize(DalConfigOptions)",
      "/Volumes/Code/p4/GoldenG/OpenGL/GLBuild/AmdRadeonLibraries/src/Latest/DalLibrary/falcon_dm/Source/AmdDalServices.cpp");
    return (unsigned int)-536870212;
  }
  v4 = this;
  if ( v3(*((_QWORD *)this + 78), "DalClamshellClosedState", &v24, 4LL) )
    *((_BYTE *)this + 534) |= v24 == 1;
  v5 = dc_create((__int64)this + 408);
  *((_QWORD *)this + 72) = v5;
  if ( !v5 )
  {
    v18 = "xxx:xxx:xxx";
    if ( *((_QWORD *)this + 73) )
      v18 = (char *)this + 608;
    v19 = "[%s][DAL] %s() !!! FAILED - dc_create() returned NULL.\n";
LABEL_27:
    kprintf(v19, v18, "initialize");
    return (unsigned int)-536870212;
  }
  v6 = AmdDalServices::initDcCallbacks(this);
  AmdDalServices::initDcDebugProperties(this, v6, v7);
  v8 = *((_QWORD *)this + 72);
  if ( *(_BYTE *)(v8 + 85) )
  {
    v21 = (AmdDalServices *)((char *)v4 + 584);
    v22 = v8;
    v23 = (a2 >> 2) & 1;
    v9 = (AmdDalDmcubService *)AmdDalDmcubService::createDmcubService(&v21);
    *((_QWORD *)v4 + 12) = v9;
    if ( v9 )
    {
      AmdDalDmcubService::initializeDmcubServices(v9);
      AmdDalDmcubService::initializeHardware(*((AmdDalDmcubService **)v4 + 12));
      if ( !(unsigned __int8)AmdDalDmcubService::isHardwareSupported(*((AmdDalDmcubService **)v4 + 12)) )
      {
        v10 = *((_QWORD *)v4 + 12);
        if ( v10 )
        {
          (*(void (**)(void))(*(_QWORD *)v10 + 8LL))();
          *((_QWORD *)v4 + 12) = 0LL;
        }
      }
      v8 = *((_QWORD *)v4 + 72);
      goto LABEL_11;
    }
    v18 = "xxx:xxx:xxx";
    if ( *((_QWORD *)v4 + 73) )
      v18 = (char *)v4 + 608;
    v19 = "[%s][DAL] %s() !!! FAILED - createDmcubService returned NULL.\n";
    goto LABEL_27;
  }
LABEL_11:
  dc_hardware_init(v8);
  *((_BYTE *)v4 + 849) = 1;
  LODWORD(v21) = 0;
  v11 = mod_color_create(*((_QWORD *)v4 + 72), &v21);
  *((_QWORD *)v4 + 107) = v11;
  v21 = 0LL;
  LOBYTE(v21) = 0;
  v22 = *((_QWORD *)v4 + 72);
  v23 = v11;
  if ( *(_DWORD *)(v22 + 28) )
  {
    LOBYTE(v12) = 0;
    do
    {
      LOBYTE(v21) = v12;
      v12 = (unsigned __int8)v12;
      *((_QWORD *)v4 + v12 + 14) = AmdDalDisplay::createDisplay(&v21);
      LOBYTE(v12) = v12 + 1;
    }
    while ( *(_DWORD *)(*((_QWORD *)v4 + 72) + 28LL) > (unsigned int)(unsigned __int8)v12 );
  }
  AmdDalDmLogger::SetLogMask(1LL, 0xFFFFFFFFLL);
  AmdDalDmLogger::SetLogMask(2LL, 0xFFFFFFFFLL);
  v13 = *((_QWORD *)v4 + 72);
  if ( *(_DWORD *)(v13 + 28) )
  {
    v14 = 0;
    do
    {
      v15 = dc_get_link_id_at_index(v13, v14);
      *((_QWORD *)v4 + (unsigned __int8)v14 + 6) = AmdDalMstService::createMstManager(
                                                     (char *)v4 + 584,
                                                     *((_QWORD *)v4 + 72),
                                                     *(_QWORD *)(*((_QWORD *)v4 + 72) + 8LL * (unsigned __int8)v14 + 848),
                                                     4LL,
                                                     (v15 >> 8) & 0xF);
      v14 = (unsigned __int8)(v14 + 1);
      v13 = *((_QWORD *)v4 + 72);
    }
    while ( *(_DWORD *)(v13 + 28) > v14 );
  }
  AmdDalI2cService::initialize(*((_QWORD *)v4 + 4));
  AmdDalAuxI2cService::initialize(*((_QWORD *)v4 + 5), *((_QWORD *)v4 + 72), v4);
  v16 = 0;
  if ( *((_BYTE *)v4 + 400) & 4 )
  {
    v21 = (AmdDalServices *)((char *)v4 + 584);
    v22 = *((_QWORD *)v4 + 72);
    v17 = (AmdDalPsrService *)AmdDalPsrService::createPsrService(&v21);
    *((_QWORD *)v4 + 13) = v17;
    if ( v17 )
    {
      AmdDalPsrService::initializePsrServices(v17);
      AmdDalPsrService::initializeHardware(*((AmdDalPsrService **)v4 + 13));
    }
  }
  return v16;
}
```

# Related issue

* https://bugzilla.kernel.org/show_bug.cgi?id=203905
