To build:

`latexmk`

Put all images in `img` and include like so:

```
\begin{figure}[htbp]
    \centering
    \includegraphics[width=\columnwidth]{img/<yourimage>}
    \caption{Caption text.}
    \label{fig:label}
\end{figure}
```

and reference like so: 

`\autoref{fig:label}`