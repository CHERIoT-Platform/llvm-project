SECTIONS {
  . = 0x1245;
  . = ALIGN(8);
  .bar : CAPALIGN {
    .bar_start = .;
	  . += 0x100000;
	  .bar_end = .;
  }
}
