# Comment-Sense Server
[![Build Status](https://travis-ci.com/Galaxyqasar/commentsense-server.svg?branch=master)](https://travis-ci.com/Galaxyqasar/commentsense-server)
## this is the server software for comment-sense

## dependencies:
- [sqlite3](https://www.sqlite.org/index.html)
- [tlse](https://github.com/eduardsui/tlse)
- [stb_image](https://github.com/nothings/stb)

## api:

- get a file:
	- method: "GET"
	- sub-url: "/*"
	- response:
		- format: auto detects format of the file based on the ending, defaults to ""
		- possible status-codes: 200, 404, 422
	- description:
		- returns the file in <./data/%url%>
		- if the file doesn't exist: 
			- status-code 404, no payload in the response
		- if the file is too big ( > 50 Mb):
			- status-code 422, payload: `{"status": "error: file too big"}` (json)
		- else:
			- status-code 200, payload == file-content
	- examples:
		- "GET /index.html ..."
		- "GET /res/icon.png ..."


- get comments:
	- method: "GET"
	- sub-url: "/api/comments"
	- arguments:  (in the url)
		- site: only get comments for the specified site and all sub-sites
			- default: any
		- count: limit comments to the first n comments
			- default: all
		- username: mark the comments whether they have been upvoted by the user
			- default: nobody
	- response:
		- format: json
		- possible status-codes: 200
		- data: `{ "comments" : [ { "id":1, "headline":"abc", "content":"def", "author":"admin", "likes":10, "voted":false, "url":"stackoverflow.com" }, ... ] }`
	- description:
		- returns the first -count- comments ordered by upvotes
		- the given url (site='url') is the start of the url the comment needs to have
		- if a username is specified the "voted" flag of the comments in the response is set to true if the user has upvoted that comment
	- examples:
		- "GET /api/comments ..."
		- "GET /api/comments?count=5&site='stackoverflow.com' ..."
		- "GET /api/comments?username='admin'&site='google.com' ..."
		- "GET /api/comments?username='admin'&site='google.com'&count=10 ..."


- get top sites:
	- method: "GET"
	- sub-url: "/api/sites"
	- arguments:  (in the url)
		- count: number of sites
			- default 5
	- response:
		- format: json
		- possible status-codes: 200
	- description:
		- returns the n most commented sites, ordered by the number of comments
	- examples:
		- "GET /api/sites ..."	// get the 5 most commented sites
		- "GET /api/sites?count=10 ..."	// get the 10 most commented sites


- post comment:
	- method: "POST"
	- sub-url: "/api/comments"
	- payload:
		- format: "json"
		- data: `{"username":"my name", "password":"my password", "sid":"1a2b3c4d5e6f789", "url":"google.com", "headline":"this is the headline", "content":"this is the content"}`
			- either username and password or the sid needs to be given
			- the sid can also be sent as a cookie
				-> username, password and sid are optional in the json, when the sid is set as a cookie
				-> if the sid is set as a cookie it overrides the sid in the json
			- if both are given, both are checked and if any of them is valid the comment is posted
	- response:
		- format: json
		- data: `{"status":"error: (wrong username or password) or (sid invalid)"}`
		- possible status-codes: 200, 500, 403
	- description:
		- post a comment with headline, content, url
		- only works if if username and password or the sid is valid


- vote comment:
	- method: "PATCH"
	- suburl: "/api/comments"
	- payload:
		- format: "json"
		- data: `{"username":"my name", "password":"my password", "sid":"1a2b3c4d5e6f789", "id":1, "vote":true}`
			- either username and password or the sid needs to be given
			- the sid can also be sent as a cookie
				-> username, password and sid are optional in the json, when the sid is set as a cookie
				-> if the sid is set as a cookie it overrides the sid in the json
			- if both are given, both are checked and if any of them is valid the comment is voted / unvoted
	- response:
		- format: json
		- data: `{"status":"error: already voted"}`
		- possible status-codes: 200, 400, 403, 500
	- description:
		- vote / unvote a comment
		- if "vote" is set to true the comment with the matching id will be upvoted
		- else it will be unvoted


- signup:
	- method: "POST"
	- suburl: "/api/signup"
	- payload:
		- format: json
		- data: `{"username":"my name", "password":"my password", "email":"my_email@sample.com"}`
			- email is optional
	- response:
		- format: json
		- data: `{"status":"signup succsessfull"}`
		- possible status-codes: 200, 403, 500
	- description:
		- create an account with specified username, password and email if the username is not already used


- signin:
	- method: "GET"
	- suburl: "/api/signin"
	- arguments: (in the url)
		- username
		- password
	- response:
		- format: json
		- header: "Set-Cookie: sid=1a2b3c4d5e6f789; PATH=/"
		- data: `{"sid":"1a2b3c4d5e6f789"}`
		- possible status-codes: 200, 403
	- description:
		- creates a session that is valid 24 hours and returns the session id (=sid)
		- the sid is also set as a cookie, which gets stored by the browser until it is closed
		- if it needs to be stored longer you can extract the sid from the json in the response
	- examples:
		- "GET /api/signin?username='my username'&password='my password' ..."


- check sid:
	- method: "GET"
	- suburl: "/api/checksid"
	- arguments: (in the url)
		- sid, only needed when not set as a cookie
	- response:
		- possible status-codes: 200, 401
	- description:
		- checks if the sid is still valid (is currently active and jounger than 24 hours)
	- examples:
		- "GET /api/checksid ..." // requires the sid to be set as a cookie
		- "GET /api/checksid?sid='1a2b3c4d5e6f789' ..."


- signout:
	- method: "GET"
	- suburl: "/api/signout"
	- arguments: (in the url)
		- sid, only needed when not set as a cookie
		- username, replaces sid
	- response:
		- possible status-codes: 200
	- description:
		- removes the current active sid from the user
		- examples:
			- "GET /api/signout ..." // requires the sid to be set as a cookie
			- "GET /api/signout?username='my username' ..."
			- "GET /api/signout?sid='1a2b3c4d5e6f789' ..."


- get user data:
	- method: "GET"
	- suburl: "/api/user"
	- arguments: (in the url)
		- sid, only needed when not set as a cookie
		- username, only when no sid is set
		- password, only when no sid is set
	- response:
		- possible status-codes: 200, 401
		- json containing the user data: `{"email":"test@test.com", "signed-in":true}`
	- description:
		- returns some account informations
		- examples:
			- "GET /api/user ..." // requires the sid to be set as a cookie
			- "GET /api/user?username='my username'&password='my password' ..."
			- "GET /api/user?sid='1a2b3c4d5e6f789' ..."


- set user data:
	- method: "PATCH"
	- suburl: "/api/user"
	- response:
		- possible status-codes: 200, 401
		- json with status
	- payload:
		- json: `{"username":"my username", "password":"my password", "email":"new email (optional)", "new-password":"my new password (optional)"}`
	- description:
		- updates the account informations