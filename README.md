UPDATE 17/02/2026: A program that uses 2048-bit key encryption together with a
multi-precision math library is out in the big-int repository:
https://github.com/abzalmyrzash/big-int

This is a program that uses the RSA public-key encryption algorithm.

To get started, you need to generate a pair of keys - one public (.pub file)
and one private (.prv file).

You will share your public key with your friend, and obviously, keep your
private key to yourself.

Your friend can then use your public key to encrypt a message, which you
can decrypt with your private key (and ideally no one else*).

Similarly, you will ask for your friend's public key to send him encrypted
messages only he* can decrypt.

Make sure that you have the required files in the program's keys directory
and have set the keys before encryption/decryption.

* No one else ideally, however this is a toy program that only generates up
  to 64-bit keys. Since the original RSA paper from 1977 recommends 200-digit
  (663-bit) keys, you can imagine how easy it is to crack a 64-bit key now.

