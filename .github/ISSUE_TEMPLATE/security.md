---
name: Security report
about: Report a security vulnerability (NOT for general bugs)
title: "[SECURITY] "
labels: ["SECURITY"]
assignees: ''
---

> **Please do not file public security issues.**  See
> [SECURITY.md](https://github.com/yuri-schmaltz/xreader_pro/blob/master/SECURITY.md)
> for the responsible-disclosure policy.  The address to use
> for private reports is listed there.

After the maintainer has acknowledged the report and a fix has
been prepared, this issue can be re-opened publicly with the
following structure:

### Vulnerability summary
* Component: (e.g. `backend/pdf/ev-poppler.cc`, `backend/dvi/`, ...)
* Severity: (low / medium / high / critical)
* Type: (buffer overflow, use-after-free, integer overflow, RCE, ...)
* Affected versions: (e.g. 4.7.x, 4.8.0)
* Discovered by: (your name / handle, optional)

### Reproduction
* Steps to reproduce
* Sample input file (DO NOT attach a real malicious file --
  describe the file format / how to construct one)
* Output / observed behaviour

### Impact
* What can an attacker do?
* What is required (user interaction, specific document type)?

### Fix
* PR / commit reference once available
* CVE number once assigned
