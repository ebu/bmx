# Repository File - Mux & Demux

Author: Markus Pfundstein, Felix Nemirovsky

This document shows how to to mux and demux Dolby Repository Files. For demonstration purposes, we will use modified version of the SOL Levante demo files from Netflix. Please refer to section Essence Creation for more information regarding the modifications.

The essence files can be found in the [Dolby Box directory](https://dolby.ent.box.com/folder/223746063813). Please request access from [Markus Pfundstein](markus.pfundstein@dolby.com).

Before following this tutorial, make sure that the Linux binary files (raw2bmx, mxf2raw) are located in your PATH.

## Muxing

In this example, we will mux one XAVC 4:2:2 Picture Track, one 5.1 24-bit WAVE as six separate PCM tracks, one IAB Stream and one Dolby Vision 2.0.5 ISXD track. All essence has been framerate converted from 24fps to 25fps with preservation of timing. 

The next table shows the tracks that we will create from the corresponding input essence.

| Track Index | Track Type | Description | Input Filename |
|-----|-----|------|------|
| 0 | Picture Track | AVC High 422 3840x2160 |  SolLevante_25fps.h264 |
| 1 | Sound Track | PCM s24le 48kHz | SolLevante_25fps.wav |
| 2 | Sound Track | PCM s24le 48kHz | SolLevante_25fps.wav |
| 3 | Sound Track | PCM s24le 48kHz | SolLevante_25fps.wav |
| 4 | Sound Track | PCM s24le 48kHz | SolLevante_25fps.wav |
| 5 | Sound Track | PCM s24le 48kHz | SolLevante_25fps.wav |
| 6 | Sound Track | PCM s24le 48kHz | SolLevante_25fps.wav |
| 7 | Sound Track | Dolby Atmos | SolLevante_25fps.iab |
| 8 | Data Track | Dolby Vision Metadata | SolLevante_25fps.isxd |

We assume that alle essence is located in the directory `~/dolby/SOL_Levante_25fps/essence_in` and the current working directory is `~/dolby/SOL_Levante_25fps`. 

To create the repository file `Repo_XAVC_IAB_ISXD_PCM51_25fps.mxf`, we execute the following command:

```
raw2bmx \
    -o SolLevante_Repository.mxf \
    -t as11op1a \
    --avc_high_422 essence_in/SolLevante_25fps.h264 \
    --isxd essence_in/SolLevante_25fps.isxd \
    --iab essence_in/SolLevante_25fps.iab \
    --wave essence_in/SolLevante_25fps.wav
```

Note that the order of arguments matters. `-o` always must comes first, followed by `-t`. Afterwards, the input essence can be specified. Here the order does not matter as the muxer always reoders to Picture - Sound - Data.

The `-t` parameter specifies the type of MXF file that we want to create. In this example, we mux a as11 compatible MXF file.

Note: Upon successful completion, you will see the following output: `Info: Duration: 6576 (00:04:23:01 @25Hz)`. As a careful tutorial follower, you will probably have inspected some of the essence (e.g. the ISXD file or the IAB file) and noted that they contain one more frame. So where did that one go? Unfortunately, the AVC stream and the WAV stream have one frame less and the muxer always using the shortest file as a reference.

## Inspection

Once the repository file has been created, we can inspect its structure and content with the `mxf2raw` command. You can also open the file with MediaInfo or MXFInspect, but as of time of writing, they were not yet able to detect ISXD and IAB essence. So you will likely see some unresolved ULs and you won't be able to inspect the EssenceDescriptors.

To inspect the file, run the following command:

```
mxf2raw SolLevante_Repository.mxf
```

You will see a quite large output. If you want to inspect the output without running the command by yourself, pleaser refer to report.txt in the Dolby Box directory referenced above.

The output displays various metadata about the Repository File. The software and company that created it, information about the file and the MXF format etc. But most importantly it displays information about the tracks and the Essence. 

### XML Output

For automatic processing, you can output xml by using the `--info-format xml` command line parameter.

## Demuxing

The last step of this tutorial shows how to demux the essence. To this end, we assume that an empty directory with name `essence_out` exists in the `~/dolby/SOL_Levante_25fps` base directory.

We will write the essence into the following essence stream files:

| Track Index | Output File |
|----|-----|
| 0 | SOL_Levante_t0_v0.h264 |
| 1 | SOL_Levante_t1_a0.pcm |
| 2 | SOL_Levante_t2_a1.pcm |
| 3 | SOL_Levante_t3_a2.pcm |
| 4 | SOL_Levante_t4_a3.pcm |
| 5 | SOL_Levante_t5_a4.pcm |
| 6 | SOL_Levante_t6_a5.pcm |
| 7 | SOL_Levante_t7_a6.iab |
| 8 | SOL_Levante_t8_d0.isxd |

The format of the output essence filename is `${PREFIX}_t${TRACK_INDEX}_${TYPE}${TYPE_INDEX}.{SUFFIX}`.

To extract the essence, run the mxf2raw command as follows:

```
mxf2raw \
    --ess-out-prefix essence_out/SOL_Levante \
    --ess-out-suffices "h264,pcm,pcm,pcm,pcm,pcm,pcm,iab,isxd" \
    SolLevante_Repository.mxf
```

The output should be a directory looking like this:

```
essence_out/
├── SOL_Levante_t0_v0.h264
├── SOL_Levante_t1_a0.pcm
├── SOL_Levante_t2_a1.pcm
├── SOL_Levante_t3_a2.pcm
├── SOL_Levante_t4_a3.pcm
├── SOL_Levante_t5_a4.pcm
├── SOL_Levante_t6_a5.pcm
├── SOL_Levante_t7_a6.iab
└── SOL_Levante_t8_d0.isxd
```

As you have probably noticed, it is currently necessary to tell the demuxer which suffices it should use for the different essence streams. There is no automatic detection yet of those. Therefore, it is recommended to first inspect a stream using XML output, determine the tracks and the essence type, and then invoke the extraction command with the appropriate prefix and suffices.

## Essence Creation

The original essence files are supplied by Netflix and can be found in their [S3 repository](http://download.opencontent.netflix.com.s3.amazonaws.com/index.html?prefix=SolLevante/)

The original essence file were 24fps and not yet available in all the formats that we needed them to be. So we had to convert them. In this section, we explain the steps that we have taken to produce the essence streams.

### XAVC

The XAVC essence has been created with BlackMagic Resolve Studio from the Netflix QuickTime essence ([Download](http://download.opencontent.netflix.com.s3.amazonaws.com/SolLevante/hdr10/SolLevante_HDR10_r2020_ST2084_UHD_24fps_1000nit.mov)). BlackMagic Resolve Studio has also converted the essence to 25fps.

### WAVE

The 5.1 WAVE file has been extracted from the aforementioned Netflix QuickTime essence with ffmpeg. We used the following command: 

```
ffmpeg \
  -i ~/Downloads/SolLevante_HDR10_r2020_ST2084_UHD_24fps_1000nit.mov \
  -r 25 \
  -vn \
  -c copy \
  ~/dolby/SOL_Levante_25fps/essence_in/SolLevante_25fps.wav
```

### IAB

To obtain a raw IAB stream, we first downloaded the Pro Tools Atmos ADM & DAMF Files ([Download](http://download.opencontent.netflix.com.s3.amazonaws.com/SolLevante/protools/ATMOS%20ADM%20%26%20DAMF%20Files.zip)). We then opened the file `sollevante_lp_v01_DAMF_Nearfield_48k_24b_24.atmos` in directory atmos/damf/sollevante_lp_v01_DAMF_Nearfield_48k_24b_24 with the Dolby Atmos Conversion Tool Version 2.1.2_07765940. 

We exported the file using the settings: 

- Filename: ~/dolby/SOL_Levante_25fps/SOL_Levante_25fps_ATMOS_IAB.mxf
- File Format: .mxf (IMF IAB)
- Frame rate: 25fps
- Sample rate: 48kHZ
- Maintain Pitch and Length: YES

The output file is a IMF compatible IAB file. To extract the raw IAB essence from this file, we used a modified version of asdcplib ([Link](n)). Please checkout branch: `feature/dump-iab-to-stream` and build it from scratch. Then invoke the command:

```
as-02-unwrap \
    ~/dolby/SOL_Levante_25fps/SOL_Levante_25fps_ATMOS_IAB.mxf \
    ~/dolby/SOL_Levante_25fps/essence_in/SolLevante_25fps.iab
```

### ISXD

The Dolby Vision ISXD file is derived from the Netflix Dolby Vision Authoring XML. ([Download](http://download.opencontent.netflix.com.s3.amazonaws.com/SolLevante/vdm/sollevante_lp_16b_hdr_p3d65pq_dolbyvision29.xml)). The first step was a framerate conversion executed by a Dolby Engineer.

We then used a custom written tool to convert the Authoring XML into a Repository XML in which the Global Metadata is wrapped together with the Frame based Metadata. This is the same process that is applied when wrapping Dolby Vision metadata into Quicktime.

The tool can be downloaded [here](https://github.com/MarkusPfundstein/dolby-dvxml2isxd) and we invoked it with the following command:

```
python -m dv2isxd \
  -i ~/dolby/SOL_Levante_25fps/sollevante_lp_16b_hdr_p3d65pq_dolbyvision29_25fps.xml \
  -o essence_in/SolLevante_25fps.isxd
```

## Development

### Directory

If you got a tarball/zip from Dolby, just unzip.

### Git

If working from git, clone the repository file to your disc and checkout the branch `feature/iab`. 

Initialize submodules with the command `git submodule update --init --remote`.

### Compilation

#### Linux

Dependencies. Please install first:

- libexpat1-dev
- libcurl4-openssl-dev
- uuid-dev
- liburiparser-dev

```
mkdir -p out/build
cd out/build
cmake ../../ -DCMAKE_BUILD_TYPE=Release
cmake --build .
#sudo make install
#sudo ldconfig
```

#### Windows (Visual Studio)

On Windows, dependencies are downloaded automatically.

```
mkdir out\build
cd out\build
cmake ..\..\
cmake --build . --config Release
#cmake --build . --config Release --target install
```

## Notes on the Repository File

The Repository File is essentially a AS11 compatible OP1A MXF. This means that we have 1 Material Package and 1 File Package and there are no cuts in any of the tracks.

Our muxer is a modified version of the BMX software developed and maintained by the BBC. Our fork has some changes that make it non-compatible with the MASTER branch of BBC. We plan to iron them out so that we can contribute back to the Open Source software. The major additions that we have made are support for IAB and ISXD muxing.

### Essence ULs in Canal+ Repository File

As of writing (and coding), the IAB and ISXD standard were still in development. We made use of the following standards to signal different essence.

#### XAVC

Reference: *SMPTE RDD32 - 2014 Table 4 & 5*

##### Essence Descriptor

We use the standard CDCI Descriptor as described in ST 377-1 with UL:
- `06.0E.2B.34.02.53.01.01.0D.01.01.01.01.01.28.00.`

##### Essence Container

| Essence Type | UL | Description 
|---------|-------------|---------|
| XAVC 3840x2160 | 06.0E.2B.34.04.01.01.0A.0D.01.03.01.02.10.60.01 | Generic Container AVC byte stream Frame Wrap |
| XAVC 1920x1080 | 06.0E.2B.34.04.01.01.0A.0D.01.03.01.02.10.60.01 | Generic Container AVC byte stream Frame Wrap |
| XAVC 1280x720 | 06.0E.2B.34.04.01.01.0A.0D.01.03.01.02.10.60.01 | Generic Container AVC byte stream Frame Wrap |

#### Essence Coding

| Essence Type | UL | Description 
|---------|-------------|---------|
| XAVC 3840x2160 4:4:4 | 06.0E.2B.34.04.01.01.0D.04.01.02.02.01.31.40.01 | AVC High Profile Unconstrained Coding |
| XAVC 3840x2160 4:2:2* | 06.0E.2B.34.04.01.01.0D.04.01.02.02.01.31.60.01 | AVC High 422 Profile Unconstrained Coding |
| XAVC 1920x1080 4:2:2 | 06.0E.2B.34.04.01.01.0D.04.01.02.02.01.31.60.01 | AVC High 422 Profile Unconstrained Coding |
| XAVC 1280x720 4:2:2 | 06.0E.2B.34.04.01.01.0D.04.01.02.02.01.31.60.01 | AVC High 422 Profile Unconstrained Coding |

*Note: DaVinci Resolve allows to assign the AVC High 422 Profile Unconstrained Coding also on AVC essence with Picture Dimensions 3840x2160. In RDD32, there is no mentioning of this combination. We added it here for completeness.

#### IAB

Reference: *TS Material Exchange Format (MXF) - Mapping Immersive Audio into the MXF Generic Container for Repository File Formats (Unpublished, Dolby Internal WD)*

##### Essence Descriptor

The Essence Descriptor in the Repository file is the same as defined in the IMF Standard document *ST 2067-201*. 

| UL | Description 
|-------------|---------|
| 06.0E.2B.34.02.7F.01.01.0D.01.01.01.01.01.7B.00 | IAB Essence Descriptor

##### Essence Container

The Essence Descriptor in the Repository file is the same as defined in the IMF Standard document *ST 2067-201*. 

| Essence Type | UL | Description 
|-|---------|-------------|
| IAB Frame Wrapped | 06.0E.2B.34.04.01.01.0D.0D.01.03.01.02.1D.00.00 | Generic Container AVC byte stream Frame Wrap | Identifies Container for Frame Wrapped IAB Sound Elements |

##### Essence Coding

| Essence Type | UL | Description 
|---------|-------------|---------|
| IAB Frame Wrapped | 06.0E.2B.34.04.01.01.05.0E.09.06.04.00.00.00.00 | Immersive Audio Coding per SMPTE ST 2098-2 |





















