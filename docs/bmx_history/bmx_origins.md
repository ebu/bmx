This information was provided by Thomas Heritage (and colleagues), co-maintainer of BMX until 2025 when the project was handed over from the BBC to the EBU. 

**Question asked:** Could you tell us something about the origins of BMX?

### libMXF (First committed December 2006)
libMXF was first committed to Sourceforge as part of the BBC R&D "Ingex" project.

* **Initial Code:** See the early commits via [GitHub](https://github.com/nexgenta/libMXF/commits/nexgenta/?after=dc7e42a98297402c5b8145e4c3359a37d7cecbff+90) (there is no link to provide for the original CVS code in SourceForge).
* **Early Production Workflows:** BBC R&D was looking at ways to improve production workflows. One aspect of this was media transfers, and specifically the use of P2 cards to transfer content between capture and editing systems. Philip de Nier (BBC R&D) began working on libMXF to read and write P2 MXF files.
* **Network Transfers & New Formats:** The next improvement to the process was supporting network transfers directly to Avid editing systems, which is where the Avid file format support came about. This was later followed by delivery formats like Sony XDCAM. This early work is covered in [WHP 155](https://downloads.bbc.co.uk/rd/pubs/whp/whp-pdf-files/WHP155.pdf) (the pdf is preserved in this repo).
* **Ingex Archive MXF:** This file format was developed around the same time as a separate strand of work. Documentation for this can be found in [WHP 167](https://downloads.bbc.co.uk/rd/pubs/whp/whp-pdf-files/WHP167.pdf), [WHP 233](https://downloads.bbc.co.uk/rd/pubs/whp/whp-pdf-files/WHP233.pdf), and [WHP 241](https://downloads.bbc.co.uk/rd/pubs/whp/whp-pdf-files/WHP241.pdf).

---

### bmx (First committed October 2011)
bmx was first committed to Sourceforge in October 2011.

* **Initial Code:** See the early commits via [GitHub](https://github.com/ebu/bmx/commits/main/?after=f81a53411a9926cbf101239d3fb413c50ecc5b1e+1550).
* **AS-02 and IMF Forerunner:** A proof-of-concept implementation had been written to test the AMWA AS-02 specification. AS-02 was a potential candidate for use in a major BBC project at the time, and a forerunner of IMF. Work had also started to specify what would become AMWA AS-11 UK DPP.
* **Open Sourcing as "bmx":** BBC R&D decided to open source the code as "bmx" (BBC MXF) with a new focus on supporting standardization efforts.
* **AS-11 Adoption:** bmx played a key role in the adoption of AS-11 by UK broadcasters. For example, it provided the core of the DPP's Metadata Application and served as a crucial engineering tool in the DPP's Compliance Programme. Read more in [Delivering File-Based Delivery](https://www.bbc.co.uk/rd/articles/2014-10-delivering-file-based-delivery).