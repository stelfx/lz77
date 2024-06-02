# LZ77 Compressor
LZ77 compression uses previously seen text as a dictionary. Duplicate strings in the input
are replaced by pointers to previous occurrences. 

The name "LZ77" comes from Lempel and Ziv, who described it in a 1977 paper.

Usage: 
1) lz77 compress [raw file name] [compressed file name]
2) lz77 decompress [compressed file name] [raw file name]
