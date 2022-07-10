<div id="top"></div>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->



<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->


<!-- PROJECT LOGO -->
<br />
<div align="center">
  <h3 align="center">Software RAID6</h3>


</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

Software RAID6 is a filesystem folder level, user mode RAID designed mainly for medical archives, but we see great potential in it for general purpose usage (i.e. reliable off-line storage, decentralized storage networks like Filecoin).

<p align="right">(<a href="#top">back to top</a>)</p>



### Built With

* extensive use of C++ 17 features
* CMake cross compiling

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started
The program takes a list of folders from the config file. The number of folders can be between 3 and 5 in the current implementation and they represent the data disks in a traditional RAID6. 
During initialization a database is built cointaining all the files of the folders assigned with a starting byte position.
The calculate step creates two additional folders, the parity folders, and fills them with the calculated values using the data folders and the database as a virtual block device.
Later on the original data folders can be restored from any N-2 surviving folders. 
The software runs on folders, the user's responsibility is to place them on separate devices or locations.

### Installation

Currently there's no binary distribution, you can run the project after cloning it using your preferred C++ compiler (compatible with C++ 17) and CMake.
The external directory should contain a version of the Jerasure project for optimized RAID6 (Galois field) calculations.

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

The program uses 'permanent directories'. That means it makes a database according to the input directory, and with that database the directory essentially becomes a block device (so you can tell e.g. the 5000-th byte in this directory is the 1234-th byte of the file 'test3.txt').
Folders are setup in the config file. They don't have to keep their physical location between calculate and recover, only they order in the config counts.

    Usage:
      raid6 calculate [--config FILE]
      raid6 recover MISSING... [--config FILE]
      raid6 init [--config FILE]
      raid6 test-files (gen|check) --number NUMBER [--config FILE]
      raid6 (-h | --help)
      raid6 --version

    Options:
      -h --help     Show this screen.
      --version     Show version.
      -c --config FILE  Path to config file [default: ./config/raid6.json].
      -n --number NUMBER  Number of files to generate for test.

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the MIT License. (So do whatever you want with it.)

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Szepes Márk - m.szepes@azp.hu

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
