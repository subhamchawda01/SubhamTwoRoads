/*
 * s3_utils.hpp
 *
 *  Created on: Aug 6, 2013
 *      Author: guest
 */

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "dvccode/CDef/email_utils.hpp"

//class DownloadErrorFlag {
// public:
//  int flag;
//
// private:
//  DownloadErrorFlag() { flag = 0; }
//
// public:
//  static DownloadErrorFlag &Instance() {
//    static DownloadErrorFlag *ins = NULL;
//    if (ins == NULL) {
//      ins = new DownloadErrorFlag();
//    }
//    return *ins;
//  }
//};
//
//inline size_t parse_header(char *ptr, size_t size, size_t nmemb, void *stream) {
//  if (!strncmp(ptr, "HTTP/1.1", 8)) DownloadErrorFlag::Instance().flag = atoi(ptr + 9);
//  return size * nmemb;
//}
//inline size_t write_curl_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
//  if (DownloadErrorFlag::Instance().flag != 200) return size * nmemb;  // do nothing else
//  size_t written;
//  written = fwrite(ptr, size, nmemb, stream);
//  return written;
//}
//
//namespace HFSAT {
//
///// IOBuf Node
//typedef struct _IOBufNode {
//  char *buf;
//  struct _IOBufNode *next;
//} IOBufNode;
//
///// IOBuf structure
//typedef struct IOBuf {
//  IOBufNode *first;
//  IOBufNode *current;
//  char *pos;
//
//  char *result;
//  char *lastMod;
//  char *eTag;
//  int contentLen;
//  int len;
//  int code;
//
//} IOBuf;
//
//class DebugFlag {
// public:
//  int debug;
//
// private:
//  DebugFlag() { debug = 0; }
//
// public:
//  static DebugFlag &Instance() {
//    static DebugFlag *ins = NULL;
//    if (ins == NULL) {
//      ins = new DebugFlag();
//    }
//    return *ins;
//  }
//};
//
//class S3Utils {
// public:
//  // IO related functions
//  // IOBuf * aws_iobuf_new ();
//  // void   aws_iobuf_append ( IOBuf *B, char * d, int len );
//  // int    aws_iobuf_getline   ( IOBuf * B, char * Line, int size );
//  // void   aws_iobuf_free ( IOBuf * bf );
//
//  int useRrs;          /// <Use reduced redundancy storage
//  char *ID;            /// <Current ID
//  char *awsKeyID;      /// <AWS Key ID
//  char *awsKey;        /// <AWS Key Material
//  const char *S3Host;  /// <AWS S3 host
//  /// \todo Use SQSHost in SQS functions
//  const char *SQSHost;  /// <AWS SQS host
//  char *Bucket;
//  char *MimeType;
//  const char *AccessControl;
//
//  static void __debug(const char *fmt, ...) {
//    /// If debug flag is not set we won't print anything
//    if (!DebugFlag::Instance().debug) return;
//    /// Otherwise process the arguments and print the result
//    va_list args;
//    va_start(args, fmt);
//    fprintf(stderr, "DBG: ");
//    vfprintf(stderr, fmt, args);
//    fprintf(stderr, "\n");
//    va_end(args);
//  }
//  static char *__aws_get_iso_date() {
//    static char dTa[256];
//    time_t t = time(NULL);
//    struct tm *gTime = gmtime(&t);
//
//    memset(dTa, 0, sizeof(dTa));
//    strftime(dTa, sizeof(dTa), "%FT%H:%M:%SZ", gTime);
//    __debug("Request Time: %s", dTa);
//    return dTa;
//  }
//
//  static char *__aws_get_httpdate() {
//    static char dTa[256];
//    time_t t = time(NULL);
//    struct tm *gTime = gmtime(&t);
//    memset(dTa, 0, sizeof(dTa));
//    strftime(dTa, sizeof(dTa), "%a, %d %b %Y %H:%M:%S +0000", gTime);
//    __debug("Request Time: %s", dTa);
//    return dTa;
//  }
//  static FILE *__aws_getcfg() {
//    int rv;
//    char ConfigFile[256];
//    /// Compose FileName and check
//    snprintf(ConfigFile, sizeof(ConfigFile) - 3, "%s/.awsAuth", getenv("HOME"));
//    __debug("Config File %s", ConfigFile);
//
//    struct stat sBuf;
//    rv = stat(ConfigFile, &sBuf);
//    if (rv == -1) return NULL;
//
//    if ((sBuf.st_mode & 066) || sBuf.st_uid != getuid()) {
//      fprintf(stderr,
//              "I refuse to read your credentials from %s as this "
//              "file is readable by, writable by or owned by someone else."
//              "Try chmod 600 %s",
//              ConfigFile, ConfigFile);
//      return NULL;
//    }
//
//    return fopen(ConfigFile, "r");
//  }
//  /*    int s3_do_get ( IOBuf *b, char * const signature,
//          char * const date, char * const resource );
//      int s3_do_put ( IOBuf *b, char * const signature,
//          char * const date, char * const resource );
//      int s3_do_delete ( IOBuf *b, char * const signature,
//          char * const date, char * const resource );
//      char* __aws_sign ( char * const str );*/
//  static void __chomp(char *str) {
//    if (str[0] == 0) return;
//    int ln = strlen(str);
//    ln--;
//    if (str[ln] == '\n') str[ln] = 0;
//    if (ln == 0) return;
//    ln--;
//    if (str[ln] == '\r') str[ln] = 0;
//  }
//
//#ifdef ENABLE_UNBASE64
//  /// Decode base64 into binary
//  /// \param input base64 text
//  /// \param length length of the input text
//  /// \return decoded data in  newly allocated buffer
//  /// \internal
//  ///
//  /// This function allocates a buffer of the same size as the input
//  /// buffer and then decodes the given base64 encoded text into
//  /// binary.   The result is placed into the allocated buffer. It is
//  /// the caller's responsibility to free this buffer
//  char *unbase64(unsigned char *input, int length) {
//    BIO *b64, *bmem;
//
//    /// Allocate and zero the buffer
//    char *buffer = (char *)malloc(length);
//    memset(buffer, 0, length);
//
//    /// Decode the input into the newly allocated buffer
//    /// \todo Check for errors during decode
//    b64 = BIO_new(BIO_f_base64());
//    bmem = BIO_new_mem_buf(input, length);
//    bmem = BIO_push(b64, bmem);
//
//    BIO_read(bmem, buffer, length);
//
//    BIO_free_all(bmem);
//
//    /// Return the decoded text
//    return buffer;
//  }
//#endif /* ENABLE_UNBASE64 */
//
//  /// Encode a binary into base64 buffer
//  /// \param input binary data  text
//  /// \param length length of the input text
//  /// \internal
//  /// \return a newly allocated buffer with base64 encoded data
//  char *__b64_encode(const unsigned char *input, int length) {
//    BIO *bmem, *b64;
//    BUF_MEM *bptr;
//
//    b64 = BIO_new(BIO_f_base64());
//    bmem = BIO_new(BIO_s_mem());
//    b64 = BIO_push(b64, bmem);
//    BIO_write(b64, input, length);
//    if (BIO_flush(b64)) {
//    }; /* make gcc 4.1.2 happy */
//    BIO_get_mem_ptr(b64, &bptr);
//
//    char *buff = (char *)malloc(bptr->length);
//    memcpy(buff, bptr->data, bptr->length - 1);
//    buff[bptr->length - 1] = 0;
//
//    BIO_free_all(b64);
//
//    return buff;
//  }
//
//  /// Handles reception of the data
//  /// \param ptr pointer to the incoming data
//  /// \param size size of the data member
//  /// \param nmemb number of data memebers
//  /// \param stream pointer to I/O buffer
//  /// \return number of bytes processed
//  struct writefunc {
//    size_t operator()(void *ptr, size_t size, size_t nmemb, void *stream) {
//      __debug("DATA RCVD %d items of size %d ", nmemb, size);
//      aws_iobuf_append((IOBuf *)stream, (char *)ptr, nmemb * size);
//      return nmemb * size;
//    }
//  };
//
//  /// Suppress outputs to stdout
//  struct writedummyfunc {
//    size_t operator()(void *ptr, size_t size, size_t nmemb, void *stream) { return nmemb * size; }
//  };
//
//  /// Handles sending of the data
//  /// \param ptr pointer to the incoming data
//  /// \param size size of the data member
//  /// \param nmemb number of data memebers
//  /// \param stream pointer to I/O buffer
//  /// \return number of bytes written
//  struct readfunc {
//    size_t operator()(const void *ptr, size_t size, size_t nmemb, void *stream) {
//      char *Ln = (char *)ptr;
//      int sz = aws_iobuf_getline((IOBuf *)stream, (char *)ptr, size * nmemb);
//      __debug("Sent[%3d] %s", sz, Ln);
//      return sz;
//    }
//  };
//
//  /// Process incming header
//  /// \param ptr pointer to the incoming data
//  /// \param size size of the data member
//  /// \param nmemb number of data memebers
//  /// \param stream pointer to I/O buffer
//  /// \return number of bytes processed
//  struct header {
//    size_t operator()(char *ptr, size_t size, size_t nmemb, void *stream) {
//      IOBuf *b = (IOBuf *)stream;
//
//      if (!strncmp(ptr, "HTTP/1.1", 8)) {
//        b->result = strdup(ptr + 9);
//        __chomp(b->result);
//        b->code = atoi(ptr + 9);
//      } else if (!strncmp(ptr, "ETag: ", 6)) {
//        b->eTag = strdup(ptr + 6);
//        __chomp(b->eTag);
//      } else if (!strncmp(ptr, "Last-Modified: ", 14)) {
//        b->lastMod = strdup(ptr + 15);
//        __chomp(b->lastMod);
//      } else if (!strncmp(ptr, "Content-Length: ", 15)) {
//        b->contentLen = atoi(ptr + 16);
//      }
//
//      return nmemb * size;
//    }
//  };
//
//#ifdef ENABLE_DUMP
//  /// Dump current state
//  /// \internal
//  void Dump() {
//    printf("----------------------------------------\n");
//    printf("ID     : %-40s \n", ID);
//    printf("KeyID  : %-40s \n", awsKeyID);
//    printf("Key    : %-40s \n", awsKey);
//    printf("S3  Host   : %-40s \n", S3Host);
//    printf("SQS Host   : %-40s \n", SQSHost);
//    printf("Bucket : %-40s \n", Bucket);
//    printf("----------------------------------------\n");
//  }
//#endif /* ENABLE_DUMP */
//
//  /// Get S3 Request signature
//  /// \internal
//  /// \param resource -- URI of the object
//  /// \param resSize --  size of the resoruce buffer
//  /// \param date -- HTTP date
//  /// \param method -- HTTP method
//  /// \param bucket -- bucket
//  /// \param file --  file
//  /// \return fills up resource and date parameters, also
//  ///         returns request signature to be used with Authorization header
//  char *GetStringToSign(char *resource, int resSize, char **date, const char *method, const char *bucket,
//                        const char *file) {
//    char reqToSign[2048];
//    char acl[32];
//    char rrs[64];
//
//    /// \todo Change the way RRS is handled.  Need to pass it in
//
//    *date = __aws_get_httpdate();
//
//    memset(resource, 0, resSize);
//    if (bucket != NULL)
//      snprintf(resource, resSize, "%s/%s", bucket, file);
//    else
//      snprintf(resource, resSize, "%s", file);
//
//    if (AccessControl)
//      snprintf(acl, sizeof(acl), "x-amz-acl:%s\n", AccessControl);
//    else
//      acl[0] = 0;
//
//    if (useRrs)
//      strncpy(rrs, "x-amz-storage-class:REDUCED_REDUNDANCY\n", sizeof(rrs));
//    else
//      rrs[0] = 0;
//
//    snprintf(reqToSign, sizeof(reqToSign), "%s\n\n%s\n%s\n%s%s/%s", method, MimeType ? MimeType : "", *date, acl, rrs,
//             resource);
//
//    // EU: If bucket is in virtual host name, remove bucket from path
//    if (bucket && strncmp(S3Host, bucket, strlen(bucket)) == 0) snprintf(resource, resSize, "%s", file);
//
//    return __aws_sign(reqToSign);
//  }
//
//  void __aws_urlencode(char *src, char *dest, int nDest) {
//    int i;
//    int n;
//    memset(dest, 0, nDest);
//    __debug("Encoding: %s", src);
//    const char *badChrs = " \n$&+,/:;=?@";
//    const char *hexDigit = "0123456789ABCDEF";
//
//    n = 0;
//    for (i = 0; src[i]; i++) {
//      if (n + 5 > nDest) {
//        puts("URLEncode:: Dest buffer to small.. can't continue \n");
//        exit(0);
//      }
//      if (strchr(badChrs, src[i])) {
//        unsigned char c = src[i];
//        dest[n++] = '%';
//        dest[n++] = hexDigit[(c >> 4) & 0xF];
//        dest[n++] = hexDigit[c & 0xF];
//      } else
//        dest[n++] = src[i];
//    }
//    __debug("Encoded To: %s", dest);
//  }
//
//  int SQSRequest(IOBuf *b, const char *verb, const char *url) {
//    CURL *ch = curl_easy_init();
//    struct curl_slist *slist = NULL;
//
//    curl_easy_setopt(ch, (CURLOPT_URL), url);
//    curl_easy_setopt(ch, (CURLOPT_HEADERDATA), b);
//    curl_easy_setopt(ch, (CURLOPT_VERBOSE), DebugFlag::Instance().debug);
//    curl_easy_setopt(ch, (CURLOPT_INFILESIZE), b->len);
//    curl_easy_setopt(ch, (CURLOPT_POST), 1);
//    curl_easy_setopt(ch, (CURLOPT_POSTFIELDSIZE), 0);
//    curl_easy_setopt(ch, (CURLOPT_HEADERFUNCTION), header());
//    curl_easy_setopt(ch, (CURLOPT_WRITEFUNCTION), writefunc());
//    curl_easy_setopt(ch, (CURLOPT_WRITEDATA), b);
//    curl_easy_setopt(ch, (CURLOPT_READFUNCTION), readfunc());
//    curl_easy_setopt(ch, (CURLOPT_READDATA), b);
//
//    int sc = curl_easy_perform(ch);
//    /** \todo check the return code  */
//    __debug("Return Code: %d ", sc);
//
//    curl_slist_free_all(slist);
//
//    return sc;
//  }
//
//  char *SQSSign(char *str) {
//    char RealSign[1024];
//    char *signature = __aws_sign(str);
//
//    __aws_urlencode(signature, RealSign, sizeof(RealSign));
//
//    free(signature);
//    return strdup(RealSign);
//  }
//
//  /*!
//\}
//   */
//
//  /*!
//\defgroup conf Configuration Functions
//\{
//   */
//
//  /// Initialize  the library
//  void aws_init() { curl_global_init(CURL_GLOBAL_ALL); }
//  /// Set debuging output
//  /// \param d  when non-zero causes debugging output to be printed
//  static void aws_set_debug(int d) { DebugFlag::Instance().debug = d; }
//
//  /// \brief Set AWS account ID to be read from .awsAuth file
//  /// \param id new account ID
//  void aws_set_id(const char *id) { ID = id == NULL ? NULL : strdup(id); }
//
//  /// Set AWS account access key
//  /// \param key new AWS authentication key
//  void aws_set_key(char *const key) { awsKey = key == NULL ? NULL : strdup(key); }
//
//  /// Set AWS account access key ID
//  /// \param keyid new AWS key ID
//  void aws_set_keyid(char *const keyid) { awsKeyID = keyid == NULL ? NULL : strdup(keyid); }
//
//  /// Set reduced redundancy storage
//  /// \param r  when non-zero causes puts to use RRS
//  void aws_set_rrs(int r) { useRrs = r; }
//
//  /// Read AWS authentication records
//  /// \param id  user ID
//  int aws_read_config(const char *const id) {
//    if (awsKeyID != NULL) return 0;
//    aws_set_id(id);
//    aws_set_keyid(NULL);
//    aws_set_key(NULL);
//
//    /// Open File
//    /// Make sure that file permissions are set right
//    __debug("Reading Config File ID[%s]", ID);
//    FILE *f = __aws_getcfg();
//    if (f == NULL) {
//      perror("Error opening config file");
//      exit(1);
//    }
//
//    /// Read Lines
//    char line[1024];
//    int ln = 0;
//    while (!feof(f)) {
//      ln++;
//      memset(line, 0, sizeof(line));
//      fgets(line, sizeof(line), f);
//
//      /// Skip Comments
//      if (line[0] == '#') continue;
//      if (line[0] == 0) continue;
//
//      __chomp(line);
//
//      /// Split the line on ':'
//      char *keyID = strchr(line, ':');
//      if (keyID == NULL) {
//        printf("Syntax error in credentials file line %d, no keyid\n", ln);
//        exit(1);
//      }
//      *keyID = 0;
//      keyID++;
//
//      char *key = strchr(keyID, ':');
//      if (key == NULL) {
//        printf("Syntax error in credentials file line %d, no key\n", ln);
//        exit(1);
//      }
//      *key = 0;
//      key++;
//
//      /// If the line is correct Set the IDs
//      if (!strcmp(line, id)) {
//        aws_set_keyid(keyID);
//        aws_set_key(key);
//        break;
//      }
//    }
//
//    if (f != NULL) fclose(f);
//    /// Return error if not found
//    if (awsKeyID == NULL) return -1;
//    return 0;
//  }
//
//  /*!
//\}
//   */
//
//  /*!
//\defgroup s3 S3 Interface Functions
//\{
//   */
//
//  /// Select current S3 bucket
//  /// \param str bucket ID
//  void s3_set_bucket(const char *str) { Bucket = str == NULL ? NULL : strdup(str); }
//
//  /// Set S3 host
//  void s3_set_host(const char *str) { S3Host = str == NULL ? NULL : strdup(str); }
//
//  /// Set S3 MimeType
//  void s3_set_mime(const char *str) { MimeType = str ? strdup(str) : NULL; }
//
//  /// Set S3 AccessControl
//  void s3_set_acl(const char *str) { AccessControl = str ? strdup(str) : NULL; }
//
//  /// Upload the file into currently selected bucket
//  /// \param b I/O buffer
//  /// \param file filename
//  int s3_put(IOBuf *b, const char *file) {
//    std::string method = "PUT";
//    char resource[1024];
//    char *date = NULL;
//
//    char *signature = GetStringToSign(resource, sizeof(resource), &date, method.c_str(), Bucket, file);
//    int sc = s3_do_put(b, signature, date, resource);
//    free(signature);
//    return sc;
//  }
//
//  /// Download the file from the current bucket
//  /// \param b I/O buffer
//  /// \param file filename
//  int s3_get(IOBuf *b, const char *file) {
//    std::string method = "GET";
//
//    char resource[1024];
//    char *date = NULL;
//
//    char *signature = GetStringToSign(resource, sizeof(resource), &date, method.c_str(), Bucket, file);
//    int sc = s3_do_get(b, signature, date, resource);
//    free(signature);
//    return sc;
//  }
//
//  /// Download the file from the current bucket and save
//  /// \param s3_path
//  /// \param download_path
//  int s3_download(const char *s3_path, FILE *downloadFile) {
//    std::string method = "GET";
//
//    char resource[1024];
//    char *date = NULL;
//
//    char *signature = GetStringToSign(resource, sizeof(resource), &date, method.c_str(), Bucket, s3_path);
//
//    char Buf[1024];
//
//    CURL *ch = curl_easy_init();
//    struct curl_slist *slist = NULL;
//
//    slist = curl_slist_append(slist, "If-Modified-Since: Tue, 26 May 2009 18:58:55 GMT");
//    slist = curl_slist_append(slist, "ETag: \"6ea58533db38eca2c2cc204b7550aab6\"");
//
//    snprintf(Buf, sizeof(Buf), "Date: %s", date);
//    slist = curl_slist_append(slist, Buf);
//    snprintf(Buf, sizeof(Buf), "Authorization: AWS %s:%s", awsKeyID, signature);
//    slist = curl_slist_append(slist, Buf);
//
//    snprintf(Buf, sizeof(Buf), "http://%s/%s", S3Host, resource);
//
//    curl_easy_setopt(ch, (CURLOPT_HTTPHEADER), slist);
//    curl_easy_setopt(ch, (CURLOPT_URL), Buf);
//    curl_easy_setopt(ch, (CURLOPT_HEADERFUNCTION), &parse_header);
//    curl_easy_setopt(ch, (CURLOPT_HEADERDATA), NULL);
//    curl_easy_setopt(ch, (CURLOPT_WRITEDATA), downloadFile);
//    curl_easy_setopt(ch, (CURLOPT_WRITEFUNCTION), &write_curl_data);
//    curl_easy_setopt(ch, (CURLOPT_VERBOSE), DebugFlag::Instance().debug);
//    curl_easy_setopt(ch, (CURLOPT_CONNECTTIMEOUT), 10);
//
//    int sc = curl_easy_perform(ch);
//    /** \todo check the return code  */
//    __debug("Return Code: %d ", sc);
//
//    //      if ( sc != CURLE_OK )
//    //        {
//    //          char hostname_ [ 128 ] ;
//    //          hostname_ [ 127 ] = '\0';
//    //          gethostname ( hostname_ , 127 );
//    //
//    //          std::string email_subject_;
//    //          {
//    //            std::ostringstream t_oss_;
//    //            t_oss_ << "s3_download Return code: " << sc
//    //                << " on " << hostname_ << "\n";
//    //
//    //            email_subject_ = t_oss_.str ( );
//    //          }
//    //
//    //          std::string email_body_;
//    //          {
//    //            std::ostringstream t_oss_;
//    //            t_oss_ << "s3_download Return code: " << sc
//    //                << " on " << hostname_ << "\n"
//    //                << " s3path: " << s3_path
//    //                << " downloadFile: " << downloadFile << "\n";
//    //
//    //            email_body_ = t_oss_.str ( );
//    //          }
//    //
//    //          HFSAT::Email email_;
//    //          email_.setSubject ( email_subject_ );
//    //          email_.addRecepient ( "hardik@circulumvite.com" );
//    //          email_.addSender ( "hardik@circulumvite.com" );
//    //          email_.content_stream << email_body_ << "<br/>";
//    //          email_.sendMail ( );
//    //        }
//
//    curl_slist_free_all(slist);
//    curl_easy_cleanup(ch);
//
//    free(signature);
//
//    return DownloadErrorFlag::Instance().flag;
//  }
//
//  /// Delete the file from the currently selected bucket
//  /// \param file filename
//  int s3_delete(IOBuf *b, char *const file) {
//    std::string method = "DELETE";
//
//    char resource[1024];
//    char *date = NULL;
//    char *signature = GetStringToSign(resource, sizeof(resource), &date, method.c_str(), Bucket, file);
//    int sc = s3_do_delete(b, signature, date, resource);
//    free(signature);
//
//    return sc;
//  }
//
//  int s3_do_put(IOBuf *b, char *const signature, char *const date, char *const resource) {
//    char Buf[1024];
//
//    CURL *ch = curl_easy_init();
//    struct curl_slist *slist = NULL;
//
//    if (MimeType) {
//      snprintf(Buf, sizeof(Buf), "Content-Type: %s", MimeType);
//      slist = curl_slist_append(slist, Buf);
//    }
//
//    if (AccessControl) {
//      snprintf(Buf, sizeof(Buf), "x-amz-acl: %s", AccessControl);
//      slist = curl_slist_append(slist, Buf);
//    }
//
//    if (useRrs) {
//      strncpy(Buf, "x-amz-storage-class: REDUCED_REDUNDANCY", sizeof(Buf));
//      slist = curl_slist_append(slist, Buf);
//    }
//
//    snprintf(Buf, sizeof(Buf), "Date: %s", date);
//    slist = curl_slist_append(slist, Buf);
//    snprintf(Buf, sizeof(Buf), "Authorization: AWS %s:%s", awsKeyID, signature);
//    slist = curl_slist_append(slist, Buf);
//
//    snprintf(Buf, sizeof(Buf), "http://%s/%s", S3Host, resource);
//
//    curl_easy_setopt(ch, (CURLOPT_HTTPHEADER), slist);
//    curl_easy_setopt(ch, (CURLOPT_URL), Buf);
//    curl_easy_setopt(ch, (CURLOPT_READDATA), b);
//    if (!DebugFlag::Instance().debug) curl_easy_setopt(ch, (CURLOPT_WRITEFUNCTION), writedummyfunc());
//    curl_easy_setopt(ch, (CURLOPT_READFUNCTION), readfunc());
//    curl_easy_setopt(ch, (CURLOPT_HEADERFUNCTION), header());
//    curl_easy_setopt(ch, (CURLOPT_HEADERDATA), b);
//    curl_easy_setopt(ch, (CURLOPT_VERBOSE), DebugFlag::Instance().debug);
//    curl_easy_setopt(ch, (CURLOPT_UPLOAD), 1);
//    curl_easy_setopt(ch, (CURLOPT_INFILESIZE), b->len);
//    curl_easy_setopt(ch, (CURLOPT_FOLLOWLOCATION), 1);
//
//    int sc = curl_easy_perform(ch);
//    /** \todo check the return code  */
//    __debug("Return Code: %d ", sc);
//
//    curl_slist_free_all(slist);
//    curl_easy_cleanup(ch);
//
//    return sc;
//  }
//
//  int s3_do_get(IOBuf *b, char *const signature, char *const date, char *const resource) {
//    char Buf[1024];
//
//    CURL *ch = curl_easy_init();
//    struct curl_slist *slist = NULL;
//
//    slist = curl_slist_append(slist, "If-Modified-Since: Tue, 26 May 2009 18:58:55 GMT");
//    slist = curl_slist_append(slist, "ETag: \"6ea58533db38eca2c2cc204b7550aab6\"");
//
//    snprintf(Buf, sizeof(Buf), "Date: %s", date);
//    slist = curl_slist_append(slist, Buf);
//    snprintf(Buf, sizeof(Buf), "Authorization: AWS %s:%s", awsKeyID, signature);
//    slist = curl_slist_append(slist, Buf);
//
//    snprintf(Buf, sizeof(Buf), "http://%s/%s", S3Host, resource);
//
//    curl_easy_setopt(ch, (CURLOPT_HTTPHEADER), slist);
//    curl_easy_setopt(ch, (CURLOPT_URL), Buf);
//    curl_easy_setopt(ch, (CURLOPT_WRITEFUNCTION), writefunc());
//    curl_easy_setopt(ch, (CURLOPT_WRITEDATA), b);
//    curl_easy_setopt(ch, (CURLOPT_HEADERFUNCTION), header());
//    curl_easy_setopt(ch, (CURLOPT_HEADERDATA), b);
//    curl_easy_setopt(ch, (CURLOPT_VERBOSE), DebugFlag::Instance().debug);
//
//    int sc = curl_easy_perform(ch);
//    /** \todo check the return code  */
//    __debug("Return Code: %d ", sc);
//
//    curl_slist_free_all(slist);
//    curl_easy_cleanup(ch);
//
//    return sc;
//  }
//
//  int s3_do_delete(IOBuf *b, char *const signature, char *const date, char *const resource) {
//    char Buf[1024];
//
//    CURL *ch = curl_easy_init();
//    struct curl_slist *slist = NULL;
//
//    snprintf(Buf, sizeof(Buf), "Date: %s", date);
//    slist = curl_slist_append(slist, Buf);
//    snprintf(Buf, sizeof(Buf), "Authorization: AWS %s:%s", awsKeyID, signature);
//    slist = curl_slist_append(slist, Buf);
//
//    snprintf(Buf, sizeof(Buf), "http://%s/%s", S3Host, resource);
//
//    curl_easy_setopt(ch, (CURLOPT_CUSTOMREQUEST), "DELETE");
//    curl_easy_setopt(ch, (CURLOPT_HTTPHEADER), slist);
//    curl_easy_setopt(ch, (CURLOPT_URL), Buf);
//    curl_easy_setopt(ch, (CURLOPT_HEADERFUNCTION), header());
//    curl_easy_setopt(ch, (CURLOPT_HEADERDATA), b);
//    curl_easy_setopt(ch, (CURLOPT_VERBOSE), DebugFlag::Instance().debug);
//
//    int sc = curl_easy_perform(ch);
//    /** \todo check the return code  */
//    __debug("Return Code: %d ", sc);
//
//    curl_slist_free_all(slist);
//    curl_easy_cleanup(ch);
//
//    return sc;
//  }
//
//  char *__aws_sign(char *const str) {
//    HMAC_CTX ctx;
//    unsigned char MD[256];
//    unsigned len;
//
//    __debug("StrToSign:%s", str);
//
//    HMAC_CTX_init(&ctx);
//    HMAC_Init(&ctx, awsKey, strlen(awsKey), EVP_sha1());
//    HMAC_Update(&ctx, (unsigned char *)str, strlen(str));
//    HMAC_Final(&ctx, (unsigned char *)MD, &len);
//    HMAC_CTX_cleanup(&ctx);
//
//    char *b64 = __b64_encode(MD, len);
//    __debug("Signature:  %s", b64);
//
//    return b64;
//  }
///*!
//\}
// */
//
// #define SQS_REQ_TAIL    
//  "&Signature=%s"       
//  "&SignatureVersion=1" 
//  "&Timestamp=%s"     
//  "&Version=2009-02-01"
//  /*!
//\defgroup sqs SQS Interface Functions
//\{
//   */
//
//  /// Create SQS queue
//  /// \param b I/O buffer
//  /// \param name queue name
//  /// \return on success return 0, otherwise error code
//  int sqs_create_queue(IOBuf *b, char *const name) {
//    __debug("Creating Que: %s\n", name);
//
//    char resource[1024];
//    char customSign[1024];
//    char *date = NULL;
//    char *signature = NULL;
//
//    const char *Req =
//        "http://%s/"
//        "?Action=CreateQueue"
//        "&QueueName=%s"
//        "&AWSAccessKeyId=%s" SQS_REQ_TAIL;
//
//    const char *Sign =
//        "ActionCreateQueue"
//        "AWSAccessKeyId%s"
//        "QueueName%s"
//        "SignatureVersion1"
//        "Timestamp%sVersion2009-02-01";
//
//    date = __aws_get_iso_date();
//    snprintf(customSign, sizeof(customSign), Sign, awsKeyID, name, date);
//    signature = SQSSign(customSign);
//
//    snprintf(resource, sizeof(resource), SQSHost, Req, name, awsKeyID, signature, date);
//
//    int sc = SQSRequest(b, "POST", resource);
//    free(signature);
//    return sc;
//  }
//
//  /// Retrieve URL of the queue
//  /// \param b I/O buffer
//  /// \param prefix queue prefix. better use the whole name
//  /// \return on success return 0, otherwise error code
//  ///
//  /// URL is placed into the I/O buffer. User get_line to retrieve it
//  int sqs_list_queues(IOBuf *b, char *const prefix) {
//    __debug("Listing Queues PFX: %s\n", prefix);
//
//    char resource[1024];
//    char customSign[1024];
//    char *date = NULL;
//    char *signature = NULL;
//
//    const char *Req =
//        "http://%s/"
//        "?Action=ListQueues"
//        "&QueueNamePrefix=%s"
//        "&AWSAccessKeyId=%s" SQS_REQ_TAIL;
//
//    const char *Sign =
//        "ActionListQueues"
//        "AWSAccessKeyId%s"
//        "QueueNamePrefix%s"
//        "SignatureVersion1"
//        "Timestamp%sVersion2009-02-01";
//
//    date = __aws_get_iso_date();
//    snprintf(customSign, sizeof(customSign), Sign, awsKeyID, prefix, date);
//    signature = SQSSign(customSign);
//
//    snprintf(resource, sizeof(resource), Req, SQSHost, prefix, awsKeyID, signature, date);
//
//    IOBuf *nb = aws_iobuf_new();
//    int sc = SQSRequest(nb, "POST", resource);
//    free(signature);
//
//    if (nb->result != NULL) b->result = strdup(nb->result);
//    b->code = nb->code;
//
//    /// \todo This only retrieves just one line in the string..
//    ///       make that all URLs are returned
//
//    if (b->code == 200) {
//      /// Parse Out the List Of Queues
//      while (-1) {
//        char Ln[1024];
//        aws_iobuf_getline(nb, Ln, sizeof(Ln));
//        if (Ln[0] == 0) break;
//        char *q = strstr(Ln, "<QueueUrl>");
//        if (q != 0) {
//          q += 10;
//          char *end = NULL;
//          end = strstr(q, "</QueueUrl>");
//          if (*end != 0) {
//            *end = 0;
//            aws_iobuf_append(b, q, strlen(q));
//            aws_iobuf_append(b, "\n", 1);
//          }
//        }
//      }
//    }
//    aws_iobuf_free(nb);
//
//    return sc;
//  }
//
//  /// Retrieve queue attributes
//  /// \param b I/O buffer
//  /// \param url queue url. Use sqs_list_queues to retrieve
//  /// \param timeOut queue visibility timeout
//  /// \param nMesg   approximate number of messages in the queue
//  /// \return on success return 0, otherwise error code
//  int sqs_get_queueattributes(IOBuf *b, char *url, int *timeOut, int *nMesg) {
//    __debug("Getting Que Attributes\n");
//
//    char resource[1024];
//    char customSign[1024];
//    char *date = NULL;
//    char *signature = NULL;
//
//    const char *Req =
//        "%s/"
//        "?Action=GetQueueAttributes"
//        "&AttributeName.1=VisibilityTimeout"
//        "&AttributeName.2=ApproximateNumberOfMessages"
//        "&AWSAccessKeyId=%s" SQS_REQ_TAIL;
//
//    const char *Sign =
//        "ActionGetQueueAttributes"
//        "AttributeName.1VisibilityTimeout"
//        "AttributeName.2ApproximateNumberOfMessages"
//        "AWSAccessKeyId%s"
//        "SignatureVersion1"
//        "Timestamp%s"
//        "Version2009-02-01";
//
//    date = __aws_get_iso_date();
//    snprintf(customSign, sizeof(customSign), Sign, awsKeyID, date);
//    signature = SQSSign(customSign);
//
//    snprintf(resource, sizeof(resource), Req, url, awsKeyID, signature, date);
//
//    const char *pfxVisTO = "<Name>VisibilityTimeout</Name><Value>";
//    const char *pfxQLen = "<Name>ApproximateNumberOfMessages</Name><Value>";
//
//    int sc = SQSRequest(b, "POST", resource);
//    while (-1) {
//      char Ln[1024];
//      aws_iobuf_getline(b, Ln, sizeof(Ln));
//      if (Ln[0] == 0) break;
//
//      char *q;
//      q = strstr(Ln, pfxVisTO);
//      if (q != 0) {
//        *timeOut = atoi(q + strlen(pfxVisTO));
//      }
//      q = strstr(Ln, pfxQLen);
//      if (q != 0) {
//        *nMesg = atoi(q + strlen(pfxQLen));
//      }
//    }
//
//    free(signature);
//    return sc;
//  }
//
//  /// Set queue visibility timeout
//  /// \param b I/O buffer
//  /// \param url queue url. Use sqs_list_queues to retrieve
//  /// \param sec queue visibility timeout
//  /// \return on success return 0, otherwise error code
//  int sqs_set_queuevisibilitytimeout(IOBuf *b, char *url, int sec) {
//    __debug("Setting Visibility Timeout : %d\n", sec);
//
//    char resource[1024];
//    char customSign[1024];
//    char *date = NULL;
//    char *signature = NULL;
//
//    const char *Req =
//        "%s/"
//        "?Action=SetQueueAttributes"
//        "&Attribute.1.Name=VisibilityTimeout"
//        "&Attribute.1.Value=%d"
//        "&AWSAccessKeyId=%s" SQS_REQ_TAIL;
//
//    const char *Sign =
//        "ActionSetQueueAttributes"
//        "Attribute.1.NameVisibilityTimeout"
//        "Attribute.1.Value%d"
//        "AWSAccessKeyId%s"
//        "SignatureVersion1"
//        "Timestamp%s"
//        "Version2009-02-01";
//
//    date = __aws_get_iso_date();
//    snprintf(customSign, sizeof(customSign), Sign, sec, awsKeyID, date);
//    signature = SQSSign(customSign);
//
//    snprintf(resource, sizeof(resource), Req, url, sec, awsKeyID, signature, date);
//
//    int sc = SQSRequest(b, "POST", resource);
//    free(signature);
//    return sc;
//  }
//
//  /// Send a message to the queue
//  /// \param b I/O buffer
//  /// \param url queue url. Use sqs_list_queues to retrieve
//  /// \param msg a message to send
//  /// \return on success return 0, otherwise error code
//  int sqs_send_message(IOBuf *b, char *const url, char *const msg) {
//    __debug("Sending Message to the queue %s\n[%s]", url, msg);
//
//    char resource[10900];
//    char customSign[10900];
//    char *date = NULL;
//    char *signature = NULL;
//    char encodedMsg[8192];
//
//    __aws_urlencode(msg, encodedMsg, sizeof(encodedMsg));
//    __debug("Encoded MSG %s", encodedMsg);
//
//    const char *Req =
//        "%s/"
//        "?Action=SendMessage"
//        "&MessageBody=%s"
//        "&AWSAccessKeyId=%s" SQS_REQ_TAIL;
//
//    const char *Sign =
//        "ActionSendMessage"
//        "AWSAccessKeyId%s"
//        "MessageBody%s"
//        "SignatureVersion1"
//        "Timestamp%s"
//        "Version2009-02-01";
//
//    date = __aws_get_iso_date();
//    snprintf(customSign, sizeof(customSign), Sign, awsKeyID, msg, date);
//    signature = SQSSign(customSign);
//
//    snprintf(resource, sizeof(resource), Req, url, encodedMsg, awsKeyID, signature, date);
//
//    int sc = SQSRequest(b, "POST", resource);
//    free(signature);
//    return sc;
//  }
//
//  /// Retrieve a message from the queue
//  /// \param b I/O buffer
//  /// \param url queue url. Use sqs_list_queues to retrieve
//  /// \param id Message receipt handle.
//  /// \return on success return 0, otherwise error code
//  ///
//  /// Message contents are placed into I/O buffer
//  /// Caller has to allocate enough memory for the receipt handle
//  /// 1024 bytes should be enough
//  int sqs_get_message(IOBuf *b, char *const url, char *id) {
//    __debug("Retieving message from: %s", url);
//
//    char resource[10900];
//    char customSign[10900];
//    char *date = NULL;
//    char *signature = NULL;
//
//    const char *Req =
//        "%s/"
//        "?Action=ReceiveMessage"
//        "&AWSAccessKeyId=%s" SQS_REQ_TAIL;
//
//    const char *Sign =
//        "ActionReceiveMessage"
//        "AWSAccessKeyId%s"
//        "SignatureVersion1"
//        "Timestamp%s"
//        "Version2009-02-01";
//
//    date = __aws_get_iso_date();
//    snprintf(customSign, sizeof(customSign), Sign, awsKeyID, date);
//    signature = SQSSign(customSign);
//
//    snprintf(resource, sizeof(resource), Req, url, awsKeyID, signature, date);
//    free(signature);
//
//    IOBuf *bf = aws_iobuf_new();
//    int sc = SQSRequest(bf, "POST", resource);
//
//    b->code = bf->code;
//    b->result = strdup(bf->result);
//
//    if (bf->code != 200) {
//      aws_iobuf_free(bf);
//      return sc;
//    }
//
//    /// \todo This is really bad. Must get a real message parser
//    int inBody = 0;
//    while (-1) {
//      char Ln[1024];
//      aws_iobuf_getline(bf, Ln, sizeof(Ln));
//      if (Ln[0] == 0) break;
//
//      __debug("%s|%s|", inBody ? ">>" : "", Ln);
//
//      char *q;
//      char *e;
//
//      /// Handle a body already being processed..
//      if (inBody) {
//        e = strstr(Ln, "</Body>");
//        if (e) {
//          *e = 0;
//          inBody = 0;
//        }
//        aws_iobuf_append(b, Ln, strlen(Ln));
//        if (!inBody) break;
//        continue;
//      }
//
//      q = strstr(Ln, "<ReceiptHandle>");
//      if (q != 0) {
//        q += 15;
//        e = strstr(Ln, "</ReceiptHandle>");
//        *e = 0;
//        strcpy(id, q);
//        q = e + 1;
//        q = strstr(q, "<Body>");
//        if (q != 0) {
//          q += 6;
//          e = strstr(q, "</Body>");
//          if (e)
//            *e = 0;
//          else
//            inBody = 1;
//          aws_iobuf_append(b, q, strlen(q));
//        }
//      }
//    }
//
//    return sc;
//  }
//
//  /// Delete processed message from the queue
//  /// \param bf I/O buffer
//  /// \param url queue url. Use sqs_list_queues to retrieve
//  /// \param receipt Message receipt handle.
//  /// \return on success return 0, otherwise error code
//  ///
//  int sqs_delete_message(IOBuf *bf, char *const url, char *receipt) {
//    char resource[10900];
//    char customSign[10900];
//    char *date = NULL;
//    char *signature = NULL;
//
//    const char *Req =
//        "%s/"
//        "?Action=DeleteMessage"
//        "&ReceiptHandle=%s"
//        "&AWSAccessKeyId=%s" SQS_REQ_TAIL;
//
//    const char *Sign =
//        "ActionDeleteMessage"
//        "AWSAccessKeyId%s"
//        "ReceiptHandle%s"
//        "SignatureVersion1"
//        "Timestamp%s"
//        "Version2009-02-01";
//
//    date = __aws_get_iso_date();
//    snprintf(customSign, sizeof(customSign), Sign, awsKeyID, receipt, date);
//    signature = SQSSign(customSign);
//
//    char encReceipt[1024];
//    __aws_urlencode(receipt, encReceipt, sizeof(encReceipt));
//
//    snprintf(resource, sizeof(resource), Req, url, encReceipt, awsKeyID, signature, date);
//    free(signature);
//
//    int sc = SQSRequest(bf, "POST", resource);
//    return sc;
//  }
//
//  /*!
//\}
//   */
//
//  /*!
//\defgroup iobuf I/O Buffer functions
//\{
//   */
//
//  /// \todo Place sentinels at the begining of the buffer
//
//  /// Create a new I/O buffer
//  /// \return a newly allocated I/O buffer
//  IOBuf *aws_iobuf_new() {
//    IOBuf *bf = (IOBuf *)calloc(sizeof(IOBuf), 1);
//
//    return bf;
//  }
//
//  /// Append data to I/O buffer
//  /// \param B  I/O buffer
//  /// \param d  pointer to the data to be appended
//  /// \param len length of the data to be appended
//  static void aws_iobuf_append(IOBuf *B, const char *d, int len) {
//    IOBufNode *N = (IOBufNode *)malloc(sizeof(IOBufNode));
//    N->next = NULL;
//    N->buf = (char *)malloc(len + 1);
//    memcpy(N->buf, d, len);
//    N->buf[len] = 0;
//    B->len += len;
//
//    if (B->first == NULL) {
//      B->first = N;
//      B->current = N;
//      B->pos = N->buf;
//    } else {
//      // Find the last block
//      IOBufNode *D = B->first;
//      while (D->next != NULL) D = D->next;
//      D->next = N;
//    }
//  }
//
//  /// Read the next line from the buffer
//  ///  \param B I/O buffer
//  ///  \param Line  character array to store the read line in
//  ///  \param size  size of the character array Line
//  ///  \return  number of characters read or 0
//  static int aws_iobuf_getline(IOBuf *B, char *Line, int size) {
//    int ln = 0;
//    memset(Line, 0, size);
//
//    if (B->current == NULL) return 0;
//
//    while (size - ln > 1) {
//      if (*B->pos == '\n') {
//        B->pos++;
//        Line[ln] = '\n';
//        ln++;
//        break;
//      }
//      if (*B->pos == 0) {
//        B->current = B->current->next;
//        if (B->current == NULL) break;
//        B->pos = B->current->buf;
//        continue;
//      }
//      Line[ln] = *B->pos;
//      ln++;
//
//      B->pos++;
//      // At the end of the block switch again
//    }
//    B->len -= ln;
//    return ln;
//  }
//
//  /// Release IO Buffer
//  /// \param  bf I/O buffer to be deleted
//  static void aws_iobuf_free(IOBuf *bf) {
//    /// Release Things
//    IOBufNode *N = bf->first;
//    if (bf->result != NULL) free(bf->result);
//    if (bf->lastMod != NULL) free(bf->lastMod);
//    if (bf->eTag != NULL) free(bf->eTag);
//    free(bf);
//
//    if (N == NULL) return;
//
//    /// Walk down the list and release blocks
//    while (N->next != NULL) {
//      IOBufNode *NN = N->next;
//      free(N);
//      N = NN;
//    }
//    if (N != NULL) free(N);
//  }
//
//  /*!
//\}
//   */
//
// private:
//  S3Utils()
//      : useRrs(0),
//        ID(NULL),
//        awsKeyID(NULL),
//        awsKey(NULL),
//        S3Host("s3.amazonaws.com"),
//        SQSHost("queue.amazonaws.com"),
//        Bucket(NULL),
//        MimeType(NULL),
//        AccessControl(0) {
//    aws_set_debug(0);
//    aws_init();
//  }
//
// public:
//  static S3Utils &GetUniqueInstance() {
//    static S3Utils *instance = NULL;
//    if (instance == NULL) {
//      instance = new S3Utils();
//    }
//    return *instance;
//  }
//};
//}
