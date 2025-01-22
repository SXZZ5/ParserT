package main

import (
	"bufio"
	"context"
	"io"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"time"
)

var launched int = 0
var finished int = 0

func muPrintln(t ...any) {
	mu.Lock()
	log.Println(t...)
	mu.Unlock()
}

func filter(str string) int {
	var res strings.Builder
	for _, r := range str {
		if r == '\r' || r == '\n' {
			continue
		}
		res.WriteRune(r)
	}

	res_str := res.String()
	ret, err := strconv.Atoi(res_str)
	if err != nil {
		muPrintln("Error converting to int")
	}
	return ret
}

var exepath string = ""

type work struct {
	input    string
	line_no  int
	test_num int
}

var mu sync.Mutex

func worker(ctx context.Context, wg *sync.WaitGroup, goodchannel chan<- *work, wrk *work) {

	defer func() {
		finished++
		wg.Done()

	}()
	cmd := exec.CommandContext(ctx, exepath)
	stdin, _ := cmd.StdinPipe()
	stdout, _ := cmd.StdoutPipe()
	stderr, _ := cmd.StderrPipe()
	defer stdin.Close()
	defer stdout.Close()
	defer stderr.Close()
	err := cmd.Start()
	if err != nil {
		muPrintln("error in starting executable", err)
		return
	}

	launched++

	stdin.Write([]byte((*wrk).input))
	readstdout, _ := io.ReadAll(stdout)

	err = cmd.Wait()

	if err == nil {
		goodchannel <- wrk
		muPrintln("i/p:", (*wrk).input)
		muPrintln("stdout :", string(readstdout))

		muPrintln(err)
		muPrintln("=====================================")
	}
}

func sender(ctx context.Context, wg *sync.WaitGroup, goodchannel chan *work,
	T int, strlist []string) {
	defer wg.Done()
	muPrintln("sender called")
	defer func() {
		muPrintln("sender exiting")
	}()
	prefixes := []string{}
	for i := 1; i <= len(strlist); i++ {
		res := ""
		if i > 1 {
			res = prefixes[i-2]
		}
		res += strlist[i-1]
		prefixes = append(prefixes, res)
	}

	muPrintln("done making prefixes")

	for i := 1; i <= T; i++ {
		tmp := strconv.Itoa(i)
		for j, l := range prefixes {
			input := tmp + "\n" + l
			one_work := work{input, j + 1, i}

			timeout_ctx, _ := context.WithTimeout(ctx, 1000*time.Millisecond)

			(*wg).Add(1)
			go worker(timeout_ctx, wg, goodchannel, &one_work)
		}
	}
}

func Filewriter(input string, test_num int) {
	filename := strconv.Itoa(test_num) + "in.txt"
	file, err := os.OpenFile(filename, os.O_CREATE|os.O_WRONLY, 0666)
	if err != nil {
		muPrintln("error opening file to write input fragment")
	}
	file.WriteString(input)
}

func waiter(ctx context.Context, goodchannel <-chan *work,
	T int, endwaiter *sync.WaitGroup, strlist []string) {
	defer endwaiter.Done()

	found := make([]bool, T)
	test_data := make([]*work, T)
	done_count := 0
Loop:
	for {
		select {
		case accepted := <-goodchannel:
			test_num := (*accepted).test_num
			if !found[test_num-1] {
				found[test_num-1] = true
				test_data[test_num-1] = accepted
				done_count++
			}

			if found[test_num-1] {
				new_test_data := (*accepted)
				if new_test_data.line_no < (*(test_data[test_num-1])).line_no {
					test_data[test_num-1] = accepted
				}
			}
		case <-ctx.Done():
			muPrintln("received context ending")
			break Loop
		}
	}

	muPrintln("test_data line no values found: ")
	muPrintln("size test_data:", len(test_data))
	for i, r := range test_data {
		muPrintln(i, (*r).line_no)
	}
	muPrintln("FINAL ISOLATED TESTS")
	old := 0
	for i := 1; i <= T; i++ {
		input := "1\n"
		for j := old + 1; j <= (*test_data[i-1]).line_no; j++ {
			input += strlist[j-1]
		}
		old = (*test_data[i-1]).line_no
		Filewriter(input, i)
	}
}

func main() {
	args := os.Args[1:]
	if len(args) > 2 {
		log.SetOutput(os.Stdout)
	} else {
		log.SetOutput(io.Discard)
	}
	file, err := os.Open(args[0])
	if err != nil {
		muPrintln("Error in opening File: ", err)
	}
	defer file.Close()

	buf := bufio.NewReader(file)
	strlist := []string{}
	for {
		line, err := buf.ReadString('\n')
		if err != nil {
			break
		}
		strlist = append(strlist, line)
	}

	str_total_tests := strlist[0]
	strlist = strlist[1:]
	T := filter(str_total_tests)
	muPrintln("T:", T)
	muPrintln("len(strlist)", len(strlist))

	exepath, err = os.Getwd()
	if err != nil {
		muPrintln("Error in getting current working directory")
	}
	exepath = filepath.Join(exepath, args[1])

	goodchannel := make(chan *work)
	ctx_workers, _ := context.WithTimeout(context.Background(), time.Second*1)
	var wg sync.WaitGroup
	wg.Add(1)
	go sender(ctx_workers, &wg, goodchannel, T, strlist)

	ctx_waiter, cancelWaiter := context.WithCancel(context.Background())
	var endwaiter sync.WaitGroup
	go waiter(ctx_waiter, goodchannel, T, &endwaiter, strlist)
	endwaiter.Add(1)

	muPrintln("going to being waiting for wg waitgroup")
	wg.Wait()
	muPrintln("done waiting for wg waitgroup")
	cancelWaiter()
	endwaiter.Wait()
	muPrintln("launched:")
	muPrintln(launched)
	muPrintln("finished:")
	muPrintln(finished)
}
