#ifndef BDN_AsyncStdioReader_H_
#define BDN_AsyncStdioReader_H_

#include <bdn/AsyncOpRunnable.h>
#include <bdn/ThreadPool.h>

namespace bdn
{

    /** Implements asynchronous reading from a std::basic_istream.
     */
    template <typename CharType> class AsyncStdioReader : public Base
    {
      public:
        /** Constructor. The implementation does NOT take ownership of the
           specified streams, i.e. it will not delete it. So it is ok to use
           std::cin here.*/
        AsyncStdioReader(std::basic_istream<CharType> *stream) : _stream(stream) {}

        /** Asynchronously reads a line of text from the stream.
            The function does not wait until the line is read - it returns
            immediately.

            Use IAsyncOp::onDone() to register a callback that is executed when
           the operation has finished.
            */
        P<IAsyncOp<String>> readLine()
        {
            P<ReadLineOp> op = newObj<ReadLineOp>(_stream);

#if BDN_HAVE_THREADS

            {
                Mutex::Lock lock(_mutex);

                // we need a thread with a queue that we can have execute our
                // read jobs one by one. We can use a thread pool with a single
                // thread for that.
                if (_opExecutor == nullptr)
                    _opExecutor = newObj<ThreadPool>(1, 1);

                _opExecutor->addJob(op);
            }
#else

            // if we do not have threading support then we must do the operation
            // synchronously. We only read a single line, so a synchronous
            // operation should be ok. However, in one case this can be
            // problematic: if there is no enough data available to the stream
            // to complete the line and eof is also not set then std::getline
            // can potentially block indefinitely. Unfortunately there is no
            // standard compliant way to prevent that, even if we were to read
            // manually. So we have to trust here that we are only used with
            // streams that have a fixed data set available to them and that
            // will set the eof flag when that data is exceeded.
            op->run();
#endif

            return op;
        }

      private:
        class ReadLineOp : public AsyncOpRunnable<String>
        {
          public:
            ReadLineOp(std::basic_istream<CharType> *stream) : _stream(stream) {}

          protected:
            String doOp() override
            {
                std::basic_string<CharType> l;

                std::getline(*_stream, l);

                return String::fromLocaleEncoding(l, _stream->getloc());
            }

          private:
            std::basic_istream<CharType> *_stream;
        };

        Mutex _mutex;
        std::basic_istream<CharType> *_stream;

#if BDN_HAVE_THREADS
        P<ThreadPool> _opExecutor;
#endif
    };
}

#endif
