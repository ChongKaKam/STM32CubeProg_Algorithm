# STM32CubeProgrammer 外部 Flash 烧录算法制作

### 目录

+ 文件介绍
+ 算法原理
+ 算法制作

<!--more-->

### 1 文件介绍

`STM32Programmer算法` 文件夹用于存放可以用于 STM32Programmer 以及 STM32CubeIDE 的烧录算法 (.stldr)

`烧录算法制作` 文件夹存放外部烧录算法制作相关资料，包括官方提供的教程以及一些 Keil 工程：

+ `算法制作工程` 存放有三个文件夹：
	+ `External_Loader_File` 存放算法制作所需要的重要文件 `Dev_inf` 以及 `Loader_Src` 以及官方给的模板代码
	+ `STM32CubeProgammerFlashAlgorithm` 存放我们所需要用到的算法制作 Keil 工程，有 HAL 库版本以及寄存器移植版本。
	+ `W25Q64_STM32H743_HAL` 被我拿来修改的 Keil 工程（没有修改名字）
+ `stm32外部烧录算法制作` 为官方提供的参考文档，虽然有点短但可以用来参考

![image-20210813010600335](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813010600335.png)

### 2 算法原理

STM32Programmer 和 STM32CubeIDE 不同于 Keil，使用的烧录算法后缀名为 .stldr（传统的烧录算法为 .elf ）因此我们如果想使用 ST 官方的软件来烧录，则需要使用 .stldr 算法文件进行烧录。

烧录算法实际上就是告诉调试器（ST-Link）如何将代码数据存放在外部 Flash 中，这样可以解决 MCU 内部 Flash 比较小无法存放执行代码以及相关文件的问题。烧录算法可以分为 **两个主要** 的部分：<u>一是设备描述（硬件接口定义）；二是存储芯片具体的控制以及读写</u>。在制作 .stldr 算法时我们需要用到的 `Dev_inf` 便是对设备的描述；`Loader_Src` 则是对于芯片通信控制的描述。通过 IDE 开发环境创建一批与地址信息无关的算法文件，实现外部 Flash 的初始化，擦除，编程，读取，校验等。在 STM32CubeProgrammer 下载阶段，系统会将算法载入芯片的内部 RAM 中，然后 STM32CubeProgrammer 通过与这个算法文件的交互，实现程序的下载和读取。

需要注意的是烧录算法不同于软件功能算法那样高度抽象的，烧录算法只适配制作算法时所给定的芯片以及引脚接口。更换芯片或者接口都会导致算法无法烧录，因此一般希望制作者在制作的时候 <u>写明适用的硬件平台以及对应的存储芯片型号</u>。

[参考网站]([(9条消息) 【STM32H7教程】第85章 STM32H7的SPI 总线应用之SPI Flash的STM32CubeProg下载算法制作_Simon223的博客-CSDN博客](https://blog.csdn.net/Simon223/article/details/110133013?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_title~default-5.base&spm=1001.2101.3001.4242)) 

### 3 算法制作

这里介绍一下制作的过程，这里使用最近在使用的 STM32H750，外部 Flash 使用支持 QSPI 的 W25Q256（存储大小为 256Mbits， 32MBytes）。W25QXX 也算是比较常见的 Flash 芯片系列，它们的控制命令基本一致，只是存储孔空间大小和划分有些差别而已。详情可以把这几个芯片的数据手册对比看看，这里不多赘述。

这里使用 STM32CubeMX + Keil 的方式来制作。如果也是使用 H7 系列的，建议直接使用例程来修改可以快速得到所需要的算法。

#### 3.1 STM32CubeMX 配置

选择 MCU 型号后进入配置，首先进行时钟树的配置：打开外部晶振（如果有的话）、配置时钟树得到合适的系统时钟速率。这里我的外部时钟是 25M Hz，如果手上的板子晶振不同的话需要手动修改。这里我配置系统频率到上限频率（480M Hz）。

:warning: 注意 AHB 的时钟数，后续配置 QSPI 时会用到。

![image-20210813012220000](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813012220000.png)

然后我们开启 QSPI，这里由于我的 Flash 芯片接在 H750 的 Bank1 上，选择 `Bank1 with Quad SPI Line`，然后进行参数设置，这里的配置需要参考 Flash 芯片的数据手册。注意的是这里 QSPI 时钟 = AHB / (Clock Prescaler + 1)。由上图的时钟树配置可以知道我的 AHB 时钟为 240M Hz，因此此时 QSPI 的时钟为 240 / (1+1) = 120 MHz。剩下的参数要根据 QSPI 时钟结核数据手册进行配置，这里不多赘述。

![image-20210813012203687](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813012203687.png)

Tips：为了实现告诉传输，需要在 GPIO 设置中将 QSPI 的引脚的速度设置为最快（very high），否则可能会导致调试失败。另外，程序中不要开启中断，改为使用查询方式。

HAL 库中关于时间基准相关的 API 也需要注释掉，直接等待就好了。无需超时处理，因为超时后意味着操作已经失败。

<img src="https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813013655935.png" alt="image-20210813013655935" style="zoom:80%;" />

然后就可以生成代码了，由于我们使用的是 Keil 来生成算法，因此我们在代码生成选项中选择 `MDK-ARM` 并选择对应的版本即可。

#### 3.2 Keil 工程生成算法

生成工程后我们需要加入4个文件：Dev_inf.c, Dev_inf.h, Loader_Src.c, Loader_Src.h

在 Core/Src 中有一个 `stm32h7xx_hal_msp.c` 该文件负责初始化引脚的，我们需要将原本的添加的头文件 “main.h” 修改成 “Loader_Src.h” 

点开 “Loader_Src.c” 文件，根据 “main.c” 中的 QSPI 初始化配置修改 Loader_Src.c 中的初始化函数。“Loader_Src.c” 中剩余的函数分别是一些读写，擦除功能的实现，可以看一看。修改后就可以把 “main.h” 移除了，因为我们后续编译中不需要用到。

“Loader_Src.h” 有许多定义可以看一下，包括 W25Q256 对应的操作指令（均可以在数据手册中查询到）以及一些存储大小 Size 的定义（如果不一样记得修改）

接下来我们点开 “Dev_inf.c” 这里为设备描述的相关信息，这里存放我们生成算法的名称、外部存储的类型、存储启示地址以及存储结构大小。这些信息都需要根据芯片型号的实际情况进行修改。

```c
struct StorageInfo const StorageInfo  =  {
#endif
   "STM32H750_W25Q256_HAL", 		// Device Name + version number
   SPI_FLASH,                       // Device Type  
   0x90000000,                      // Device Start Address
   0x02000000,                 	    // Device Size in Bits (0x2000000 = 33554432 bits = 32MBytes)
   0x100,                 			// Programming Page Size 256 Bytes
   0xFF,                       		// Initial Content of Erased Memory
// Specify Size and Address of Sectors (view example below)
   0x00002000, 0x00001000,     				 		// Sector Num : 8192 ,Sector Size: 4KBytes 
   0x00000000, 0x00000000,
};
```

修改之后就可以点击编译进行 Debug，没有报错后说明算法已经差不多了，接下来需要进行一些设置：

1. 修改生成的算法名称：

![image-20210813020813250](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813020813250.png)

2. 保证生成的算法文件中 RO 和 RW 段的独立性（与地址无关，Position Independent）：

	C/C++ 编译：

	![image-20210813021611464](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813021611464.png)

	汇编：

	![image-20210813022640894](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813022640894.png)

3. 将程序可执行文件 axf 修改为 stldr 格式：

	![image-20210813022854723](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813022854723.png)

	User Command：

	```
	cmd.exe /C copy "!L" ".\@L.stldr"

4. 分散加载设置：

	![image-20210813023113553](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813023113553.png)

	Target.lin 为链接控制文件（Link Control File），可以在例程中找到。下面为 Target.lin 的具体内容：

	```
	FLASH_LOADER 0x20000004 PI   ; FlashLoader Functions
	{
	  PrgCode +0           ; Code
	  {
	    * (+RO)
	  }
	  PrgData +0           ; Data
	  {
	    * (+RW,+ZI)
	  }
	}
	
	DEVICE_INFO +0               ; Device Info
	{
	  DevInfo +0           ; Info structure
	  {
	    dev_inf.o
	  }
	}
	```

	这里要修改下Flash算法加载地址，将 0x20000004 修改为 STM32H7 的 RAM 地址，任何 RAM 块地址均可，只要够存储Flash算法。推荐设置为 AXI SRAM 地址 0x24000004，因为空间够大，有 512KB。

	`--diag_suppress L6305` 用于屏蔽 L6503 类型警告信息。

设置完后重新编译，就可以在  /MDK-ARM 文件夹中得到我们所需要的 .stldr 烧录算法文件了。

### 4 算法使用

完成了算法制作后，我们可以利用 STM32CubeProgrammer 来进行烧录，首先需要将我们的烧录算法文件放在对应的文件夹中：`\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\ExternalLoader` ，这样才能在 STM32CubeProgrammer 中找到我们制作的算法。

<img src="https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813024832041.png" alt="image-20210813024832041" style="zoom:80%;" />

勾选后，就可以打开左上方选项（Memory & File edition），点击 open file 打开放置在外部 Flash 程序的 .hex 文件，最后点击 Dowload 进行烧录。

![image-20210813025141876](https://gitee.com/zhuang-jiaxin_gitee/pictures/raw/master/image-20210813025141876.png)

没有报错结束后，会提示烧录成功，可以看一下板子是不是正常运行。

Tips：如果 ST-Link 版本过低也可能导致烧录失败，可以点击 `Firmware upgrade` 来升级一下 ST-Link 的固件。

---

[Github 仓库链接](https://github.com/ChrisZ-NJU/STM32CubeProg_Algorithm.git)
