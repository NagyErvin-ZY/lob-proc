{{- define "viz.fullname" -}}
{{- .Release.Name | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "viz.labels" -}}
app.kubernetes.io/name: viz
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}

{{- define "viz.selectorLabels" -}}
app.kubernetes.io/name: viz
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}
