{{- define "processor.fullname" -}}
{{- .Release.Name | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "processor.labels" -}}
app.kubernetes.io/name: processor
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}

{{- define "processor.selectorLabels" -}}
app.kubernetes.io/name: processor
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}
